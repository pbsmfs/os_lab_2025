for i in {1..150}; do
    num=$(od -An -N2 -i /dev/random)
    echo $num
done > numbers.txt

