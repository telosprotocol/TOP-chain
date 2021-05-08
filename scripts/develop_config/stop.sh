kill -9 `ps -ef |grep xtopchain |grep -v 'sh' |grep -v grep|awk '{print $2}'`
kill -9 `ps -ef |grep topio |grep -v 'sh' |grep -v grep|awk '{print $2}'`
