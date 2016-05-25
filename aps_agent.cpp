#define __STDC_FORMAT_MACROS 1
#include <inttypes.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <time.h>

#include <iostream>
#include <string>
#include <algorithm>

#include <map>

#include <pthread.h>

#include <sys/socket.h>
#include <dirent.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <errno.h>
#include <pwd.h>

#include <unistd.h>
#include <fcntl.h>

#include <linux/limits.h>

void* input_and_process_thread(void*);
void clean_log_files(const char* dir);


const char * SERVER_FILENAME = "APS_AGENT.AF_UNIX.SOCK_DGRAM";

int g_input_unix_sock_fd = 0;

struct sta_data{
  uint64_t count;
  uint64_t success_count;
  uint64_t time_us;
  uint64_t success_time_us;
  std::map<int64_t, uint64_t> sta_error_code;
};

typedef std::map<time_t /* timestamp */, std::map<std::string /* sys1 module1 sys2 module2 ip2 port2 saperated by tab*/, sta_data> > sta_data_map;
sta_data_map g_sta_data_by_seconds;

pthread_mutex_t g_mutex;

// unix socket directory permission should be 755 so that socket file cannot be moved
// unix socket permission, 777
// data directory permission, 755
// data log file permission, 644
// ./aps_agent  unix_socket_directory data_directory
int main(int argc, char *argv[]) {

  // if(argc<2) {
  //   fprintf(stderr,"./aps_agent unix_socket_directory\n");
  //   return 1;
  // }

  const char* username = "aps_agent";
  if(argc>=2) {
    username = argv[1];
  }
  const char* socket_dir = "/run/aps_agent";
  if(argc>=3) {
    socket_dir = argv[2];
  }
  const char* data_dir = "/var/log/aps_agent";
  if(argc>=4) {
    data_dir = argv[3];
  }
  printf("username[%s] socket_dir[%s] data_dir[%s]\n",username,socket_dir,data_dir);

  passwd* pw;
  if((pw = getpwnam(username)) == NULL) {
    char error_msg[1024];
    snprintf(error_msg,sizeof(error_msg), "Userid '%s' does not exist", username);
    perror(error_msg);
    exit(1);
  }
  if (setgid(pw->pw_gid) != 0) {
    char error_msg[1024];
    snprintf(error_msg ,sizeof(error_msg), "setgid() to %d(%s) failed", pw->pw_gid, username);
    perror(error_msg);
    exit(1);
  }
  if (setuid(pw->pw_uid) != 0) {
    char error_msg[1024];
    snprintf(error_msg ,sizeof(error_msg), "setuid() to %d(%s) failed", pw->pw_uid, username);
    perror(error_msg);
    exit(1);
  }

  daemon(1,0); // no change to root dir

  if ( (g_input_unix_sock_fd = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0) {
    perror("cannot create socket");
    return 0;
  }

  struct sockaddr_un srv_un = {0};
  srv_un.sun_family = AF_UNIX;
  snprintf(srv_un.sun_path, sizeof(srv_un.sun_path), "%s/%s", socket_dir, SERVER_FILENAME);
  /*If you leave the file behind when you're finished, or perhaps crash after binding, the next bind will fail
     with "address in use". Which just means, the file is already there.*/
  unlink(srv_un.sun_path);

  if (bind(g_input_unix_sock_fd, (struct sockaddr *)&srv_un, sizeof(srv_un)) == -1) {
    perror("bind server");
    exit(1);
  }

  if(chmod(srv_un.sun_path, 0666)<0) {
    char msg[1024] = {0};
    snprintf(msg,sizeof(msg)-1,"chmod (%s,0666) failed.",srv_un.sun_path);
    perror("chmod failed");
  }


  pthread_mutex_init(&g_mutex,NULL);

  pthread_t  thread;
  int ret = pthread_create(&thread, 0, &input_and_process_thread, NULL);
  if(ret<0) {
    perror("pthread_create error");
    exit(2);
  }

  sta_data_map sta_by_min;

  while(1) {

    sta_data_map tmp_map; // buffer for output

    sleep(1);

    clean_log_files(data_dir);

    pthread_mutex_lock(&g_mutex);

    time_t now = time(NULL);

    sta_data_map::iterator it = g_sta_data_by_seconds.begin();
    while((it!=g_sta_data_by_seconds.end()) && (it->first<now)) {
      ++it;
    }

    // move to tmp_map
    tmp_map.insert(g_sta_data_by_seconds.begin(),it);
    g_sta_data_by_seconds.erase(g_sta_data_by_seconds.begin(),it);

    pthread_mutex_unlock(&g_mutex);

    // stat by minute
    for(sta_data_map::iterator time_it = tmp_map.begin(); time_it!=tmp_map.end(); ++time_it) {

      time_t time_by_min = time_it->first - (time_it->first % 60);

      for(std::map<std::string, sta_data>::iterator point_it = time_it->second.begin();
          point_it!=time_it->second.end();
          ++point_it) {

        sta_data& data_sec = point_it->second;
        sta_data& data_min = sta_by_min[time_by_min][point_it->first];
        data_min.count           += data_sec.count;
        data_min.success_count   += data_sec.success_count;
        data_min.time_us         += data_sec.time_us;
        data_min.success_time_us += data_sec.success_time_us;

        // error code
        for(std::map<int64_t, uint64_t>::iterator code_it = data_sec.sta_error_code.begin(); code_it!=data_sec.sta_error_code.end(); ++code_it) {
          data_min.sta_error_code[code_it->first] += code_it->second;
        }

      }
    }

    // output sec stat to terminal
    for(sta_data_map::iterator time_it = tmp_map.begin(); time_it!=tmp_map.end(); ++time_it) {

      // data_path
      char stat_sec_path[PATH_MAX];

      time_t t = time_it->first;
      tm result;
      localtime_r(&t,&result);
      char time_Ymd[64];
      strftime(time_Ymd,sizeof(time_Ymd),"%Y%m%d",&result);
      snprintf(stat_sec_path,sizeof(stat_sec_path),"%s/stat_sec-%s.log",data_dir,time_Ymd);

      int fd_stat_sec = open(stat_sec_path,O_APPEND|O_WRONLY|O_CREAT,0644);
      fchmod(fd_stat_sec,0644);

      char tmp_write_buf[1024];

      for(std::map<std::string, sta_data>::iterator point_it = time_it->second.begin();
          point_it!=time_it->second.end();
            ++point_it) {

        const std::string& key = point_it->first;
        const sta_data& d      = point_it->second;

        char time_Ymd_HMS[64];
        struct tm* tm_info = localtime(&(time_it->first));
        strftime(time_Ymd_HMS, 26, "%Y-%m-%d %H:%M:%S", tm_info);

        size_t len = snprintf(tmp_write_buf,sizeof(tmp_write_buf),"%s" "\t%s" "|" "%"PRIu64 "\t%"PRIu64 "\t%"PRIu64 "\t%"PRIu64 "\n"
              , time_Ymd_HMS, key.c_str(), d.count, d.success_count, d.time_us, d.success_time_us
            );

        write(fd_stat_sec,tmp_write_buf,len);

        // printf("%s" "\t%s" "\t%"PRIu64 "\t%"PRIu64 "\t%"PRIu64 "\t%"PRIu64 "\t%.2f%%" "\t%.2f" "\n"
        //       , time_Ymd_HMS, point_it->first.c_str()
        //       , point_it->second.count, point_it->second.success_count, point_it->second.time_us, point_it->second.success_time_us
        //       , (double)(((double)(point_it->second.success_count*100))/point_it->second.count), ((double)point_it->second.time_us)/point_it->second.count
        //     );

      }

      close(fd_stat_sec);

    }

    // output min stat to terminal
    for(sta_data_map::iterator time_it = sta_by_min.begin(); time_it!=sta_by_min.end(); ++time_it) {

      time_t t = time_it->first;

      if(t > now-60) { // data not ready
        break;
      }

      // data_path
      char stat_min_path[PATH_MAX];
      char code_min_path[PATH_MAX];

      tm result;
      localtime_r(&t,&result);
      char time_Ymd[64];
      strftime(time_Ymd,sizeof(time_Ymd),"%Y%m%d",&result);

      snprintf(stat_min_path,sizeof(stat_min_path),"%s/stat_min-%s.log",data_dir,time_Ymd);
      snprintf(code_min_path,sizeof(stat_min_path),"%s/code_min-%s.log",data_dir,time_Ymd);

      int fd_stat_min = open(stat_min_path,O_APPEND|O_WRONLY|O_CREAT,0644);
      int fd_code_min = open(code_min_path,O_APPEND|O_WRONLY|O_CREAT,0644);
      fchmod(fd_stat_min,0644);
      fchmod(fd_code_min,0644);

      char tmp_write_buf[1024];

      for(std::map<std::string, sta_data>::iterator point_it = time_it->second.begin();
          point_it!=time_it->second.end();
            ++point_it) {

        const std::string& key = point_it->first;
        const sta_data& d      = point_it->second;

        char time_Ymd_HMS[64];
        struct tm* tm_info = localtime(&(time_it->first));
        strftime(time_Ymd_HMS, 26, "%Y-%m-%d %H:%M", tm_info);

        size_t len = snprintf(tmp_write_buf,sizeof(tmp_write_buf),"%s" "\t%s" "|" "%"PRIu64 "\t%"PRIu64 "\t%"PRIu64 "\t%"PRIu64 "\n"
              , time_Ymd_HMS, key.c_str(), d.count, d.success_count, d.time_us, d.success_time_us
            );

        write(fd_stat_min,tmp_write_buf,len);

        // output code
        for(std::map<int64_t,uint64_t>::const_iterator code_it=d.sta_error_code.begin(); code_it!=d.sta_error_code.end(); ++code_it) {
          size_t len = snprintf(tmp_write_buf,sizeof(tmp_write_buf),"%s" "\t%s" "\t%"PRIi64 "|" "%"PRIu64 "\n"
                , time_Ymd_HMS, key.c_str(), code_it->first, code_it->second
              );
          write(fd_code_min,tmp_write_buf,len);
        }

      }

      close(fd_stat_min);
      close(fd_code_min);

    }

    for(sta_data_map::iterator time_it = sta_by_min.begin(); time_it!=sta_by_min.end(); ) {
      time_t t = time_it->first;
      if( (t > now-60) || (++time_it==sta_by_min.end()) ) { // data not ready
        sta_by_min.erase(sta_by_min.begin(),time_it);
        break;
      }

    }

  }

}

void clean_log_files(const char* dir) {

  time_t now = time(NULL);

  char stat_sec_path[PATH_MAX];

  DIR *dp = opendir(dir);
  if(dp==NULL) {
    char msg[2048];
    snprintf(msg,sizeof(msg),"open directory[%s] failed.",dir);
    perror(msg);
    return ;
  }

  struct dirent *ep;
  while (ep = readdir (dp)) {

    const char* name = ep->d_name;
    char path[10*1024];
    snprintf(path,sizeof(path),"%s/%s",dir,name);

    time_t file_time = 0;

    struct tm st_time;
    memset(&st_time, 0, sizeof(struct tm));
    if(strstr(name,"stat_sec-")==name) {
      strptime(name, "stat_sec-%Y%m%d.log", &st_time);
      file_time = mktime(&st_time);
    } else if(strstr(name,"code_min-")==name) {
      strptime(name, "code_min-%Y%m%d.log", &st_time);
      file_time = mktime(&st_time);
    } else if(strstr(name,"stat_min-")==name) {
      strptime(name, "stat_min-%Y%m%d.log", &st_time);
      file_time = mktime(&st_time);
    } else{
      continue;
    }

    uint64_t size = 0;
    struct stat buf;
    if(stat(path, &buf)==0) {
      size = buf.st_size;
    }

    if(strstr(name,"stat_sec-")==name) {
      if(now-file_time > 60*60*24) {
        unlink(path);
        continue;
      } else if(size>1024*1024*200) {
        unlink(path);
        continue;
      }
    } else {
      if(now-file_time > 60*60*24*7) {
        unlink(path);
        continue;
      }
    }

  }

  closedir (dp);
}

void *input_and_process_thread(void*) {

  const int BUF_SIZE_LIMIT = 512;

  char raw_buf[BUF_SIZE_LIMIT+1]   = {0};
  char key_buf[BUF_SIZE_LIMIT+1]   = {0};
  char dummy_buf[BUF_SIZE_LIMIT+1] = {0};

  uint64_t time_us = 0;
  int result   = 1;
  int64_t code     = 0;

  struct sockaddr_un client_address;
  socklen_t address_length = sizeof(struct sockaddr_un);

  std::string key;

  while(1) {

    raw_buf[0] = '\0';
    key_buf[0] = '\0';

    time_us = 0;
    result  = 1;
    code    = 0;

    // recv 
    int n = recvfrom(g_input_unix_sock_fd, raw_buf, BUF_SIZE_LIMIT, 0, (struct sockaddr *) &(client_address),&address_length);
    if(n<0) {
      perror("receivefrom failed.");
      break;
    }
    raw_buf[n] = '\0'; // protect sscanf

    // for debug
    // std::cerr<<"received ["<<raw_buf<<"]"<<std::endl
    //   <<"len:"<<n<<std::endl;

    // check key data and statistic data
    int scan_ret = sscanf(raw_buf, "%[^|]|%"PRIu64"\t%i\t%"PRIi64, key_buf,&time_us,&result,&code);
    if(scan_ret<4 || scan_ret==EOF) {
      // invalid format
      continue;
    }

    // verify key format
    int key_scan = sscanf(key_buf,"%[^\t]\t%[^\t]\t%[^\t]\t%[^\t]\t%[^\t]\t%[^\t]"
        ,dummy_buf,dummy_buf,dummy_buf,dummy_buf,dummy_buf,dummy_buf);
    if((key_scan<6) || (key_scan==EOF)) {
      // invalid format
      continue;
    }

    // statistic
    
    key = key_buf;

    pthread_mutex_lock(&g_mutex);

    sta_data& d = g_sta_data_by_seconds[time(NULL)][key];

    d.count += 1;
    d.time_us += time_us;
    if(result)
    {
      d.success_count += 1;
      d.success_time_us += time_us;
    }
    d.sta_error_code[code]++;

    pthread_mutex_unlock(&g_mutex);
  }

  return NULL;
}


