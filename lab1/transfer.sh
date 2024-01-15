unset sender
unset receiver
unset encoder
unset decoder

case $1 in
        "--sender"                 | "-s"   ) sender=1;;
        "--receiver"               | "-r"   ) receiver=1;;
        "--encoder"                | "-e"   ) encoder=1;;
        "--decoder"                | "-d"   ) decoder=1;;
esac

if [[ $encoder ]]; then
        # echo "encoder"
        if [ $# -ne 2 ]; then
                echo "Please enter the transferred file name."
        else
                base64  $2 > ./base64_encoded_file.txt
        fi
elif [[ $sender ]]; then
        # echo "sender"
        sudo ./sender.o
elif [[ $receiver ]]; then
        # echo "receiver"
        ./receiver.o base64_encoded_file.txt
elif [[ $decoder ]]; then
        # echo "decoder"
        base64 -d base64_encoded_file.txt > a.out
        chmod 777 a.out
else
        echo "usage: transfer.sh [--sender | --receiver]"
fi