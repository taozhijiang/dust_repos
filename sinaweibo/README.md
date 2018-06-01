# sinaweibo
调用新浪微博的开放接口，定制自己的微博机器人

依赖于weibo，请安装pip install weibo

web_cfg由于有私有文件，没有公开。可以自己申请SINA APP账号，或者私下向我索取。
其中需要的配置参数有
APP_KEY = 
APP_SECRET = 
CALLBACK = 'https://api.weibo.com/oauth2/default.html'
AUTH_URL = 'https://api.weibo.com/oauth2/authorize'

UID = 1683951363  #网页登陆你的weibo账号就可以看到了

USER = 'weibo账号'
PASSWD = 'weibo密码'

TTFILE = os.getcwd()+"/toutiao.txt"

运行方式 python run.py。如有有服务器，建议添加到crontab按时自动运行

项目的定制目标：
(1)根据Twitter的开放接口，将Twitter上的“和谐”消息，在墙外的服务器自动调用
Sina微博转发；
(2)跟踪一些好的网站，爬虫后微博分享。

20150512 
    目前抓取toutiao.io，然后自动发送到微博上。
	
20150513
	日志变为英文，否则crontab无法执行
	crontab格式
	40 5 * * *    /bin/bash /home/user/project/sinaweibo/start.sh
	
20150514
	程序分开，repost没两个小时执行一次，toutiao每天执行一次
	转发的有白名单的限制
	# m h  dom mon dow   command
	40 */2 * * *    /bin/bash /home/user/project/sinaweibo/start.sh repost
	15 5 * * *    /bin/bash /home/user/project/sinaweibo/start.sh toutiao

20150515
	添加Twitter转发模块。不过windows正常，Linux不正常。。。
