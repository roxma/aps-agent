Name:		aps-agent
Version:	1.0.0
Release:	1%{?dist}
Summary:	Application Performance Statistic Agent
BuildRoot: /var/tmp/%{name}-buildroot

Group:		Development/Tools
License:	MIT
URL:		https://github.com/roxma/aps-agent
Source0:	https://github.com/roxma/aps-agent/archive/master.zip


%description
This is an application performance statistic tool.

%prep
# pwd
# cd "$RPM_BUILD_DIR" && rm -rf *
# wget https://github.com/roxma/aps-agent/archive/master.zip
# unzip master.zip
# mv aps-agent-master aps-agent
cd "$RPM_BUILD_DIR" && rm -rf *
cd "$RPM_BUILD_DIR" && cp -r %{_my_source_code_dir}/* ./


%build
# cd aps-agent
# make
make


%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/usr/local/aps-agent/bin
install bin/aps-agent  $RPM_BUILD_ROOT/usr/local/aps-agent/bin
install stat_print.py  $RPM_BUILD_ROOT/usr/local/aps-agent/bin
mkdir -p $RPM_BUILD_ROOT/var/log/aps-agent/
mkdir -p $RPM_BUILD_ROOT/run/aps-agent/


%files
%attr(755,aps-agent,aps-agent) /usr/local/aps-agent
%attr(755,aps-agent,aps-agent) /usr/local/aps-agent/bin
%attr(755,aps-agent,aps-agent) /usr/local/aps-agent/bin/aps-agent
%attr(755,aps-agent,aps-agent) /usr/local/aps-agent/bin/stat_print.py
%attr(755,aps-agent,aps-agent) /var/log/aps-agent/
%attr(755,aps-agent,aps-agent) /run/aps-agent/

%pre
/usr/bin/getent group aps-agent || /usr/sbin/groupadd -r aps-agent
/usr/bin/getent passwd aps-agent || /usr/sbin/useradd -r -d /usr/local/aps-agent/ -g aps-agent -s /sbin/nologin aps-agent

# %postun
# /usr/sbin/userdel myservice


%changelog

