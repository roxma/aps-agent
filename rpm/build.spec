Name:		aps_agent
Version:	1.0.0
Release:	1%{?dist}
Summary:	Application Performance Statistic Agent
BuildRoot: /var/tmp/%{name}-buildroot

Group:		Development/Tools
License:	MIT
URL:		https://github.com/roxma/aps_agent
Source0:	https://github.com/roxma/aps_agent/archive/master.zip


%description
This is an application performance statistic tool.

%prep
# pwd
# cd "$RPM_BUILD_DIR" && rm -rf *
# wget https://github.com/roxma/aps_agent/archive/master.zip
# unzip master.zip
# mv aps_agent-master aps_agent
cd "$RPM_BUILD_DIR" && rm -rf *
cd "$RPM_BUILD_DIR" && cp -r %{_my_source_code_dir}/* ./


%build
# cd aps_agent
# make
make


%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/usr/local/aps_agent/bin
install bin/aps_agent  $RPM_BUILD_ROOT/usr/local/aps_agent/bin
install stat_print.py  $RPM_BUILD_ROOT/usr/local/aps_agent/bin
mkdir -p $RPM_BUILD_ROOT/var/log/aps_agent/
mkdir -p $RPM_BUILD_ROOT/run/aps_agent/


%files
%attr(755,aps_agent,aps_agent) /usr/local/aps_agent
%attr(755,aps_agent,aps_agent) /usr/local/aps_agent/bin
%attr(755,aps_agent,aps_agent) /usr/local/aps_agent/bin/aps_agent
%attr(755,aps_agent,aps_agent) /usr/local/aps_agent/bin/stat_print.py
%attr(755,aps_agent,aps_agent) /var/log/aps_agent/
%attr(755,aps_agent,aps_agent) /run/aps_agent/

%pre
/usr/bin/getent group aps_agent || /usr/sbin/groupadd -r aps_agent
/usr/bin/getent passwd aps_agent || /usr/sbin/useradd -r -d /usr/local/aps_agent/ -g aps_agent -s /sbin/nologin aps_agent

# %postun
# /usr/sbin/userdel myservice


%changelog

