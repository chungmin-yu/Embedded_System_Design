unset sender
unset receiver

case $1 in
	"--sender"                 | "-s"   ) sender=1;;
	"--receiver"               | "-r"   ) receiver=1;;
esac

if [[ $sender ]]; then
    # echo "sender"
	if [ $# -ne 2 ]; then
		echo "Please enter the transferred file name."
	else
		base64  $2 > ./base64_encoded_file.txt
		sudo ./sender.o
	fi
elif [[ $receiver ]]; then
	# echo "receiver"
	cat > base64_encoded_file.txt
	base64 -d base64_encoded_file.txt > a.out
	chmod 777 a.out
else
	echo "usage: transfer.sh [--sender | --receiver]"
fi