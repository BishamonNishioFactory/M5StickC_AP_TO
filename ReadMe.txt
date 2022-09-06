##################################################################################
#
#	M5StickC　仕上混流あんどん＆検査明示＆Firebase送信　プログラム
#　　　　A.Yamazaki 2022 Aug
#
##################################################################################

git config --global user.name  とuser.emailを変更したら、push が出来なくなった。
以下で成功した。

git push https://BishamonNishioFactory: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@github.com/BishamonNishioFactory/M5StickC_AP_MX.git    



GitHub　で本格管理していく。

目的
これまでは山崎が単独でプログラミングしてきたが、このままでは趣味の領域を脱することが出来ない。
今後は、大地GMや大島にプログラミングを習得してもらって、組織として運営していきた。
チームでプログラム開発していく上では、GitHub は欠かすことの出来ないツールである。

pushは、編集者自らのユーザー情報で行うこと。
git config --global user.name honyarara
git config --global user.email honyaraar
予めGitHubのアカウントを作っておくことのが望ましい。
（プロフィール情報で編集者が特定出来るため）

2022/9/6　AYamazaki
　



下記カスタマイズ項目で、対象設備名と、Slack接続key（Incomming-webhook）を適宜変更すると、どこでも使えるようになる。



カスタマイズ項目
画面表示及びFirebase 送信の対象設備名
Slackへの接続key
Wifi接続先


Gmailアカウント
及び
GitHubは、

nisio2221@gmail.com

Password、
nisio12345678


アクセストークン（パスワードを求められたらこれを入力する）
 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


2nd token
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

なぜか認証されなくなったので、トークンを再発行したところ、コピペでうまくいった!
8/26 16:30


W640 でcloneを実行したところ、自動的に同盟のフォルダーが作成された。
もう一度、MacBookPro で本文を記述編集すると、どのように振る舞うのかを検証する。２０２２年８月２６日午後１３：２６
