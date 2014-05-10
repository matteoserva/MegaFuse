touch test1.txt && ls test1.txt > /dev/null && rm test1.txt && ls *.txt 2>/dev/null || echo test 1 succesful

echo "test 2 successful"> test1.txt && echo "test2 failed" > test2.txt && mv test1.txt test2.txt && ls test1.txt 2>/dev/null|| cat test2.txt && rm test2.txt
ls *.txt 2> /dev/null && echo check failed

echo "failed failed failed failed" > test1.txt && echo "test 3 successful" > test1.txt && cat test1.txt && rm test1.txt

touch test.txt && ls test.txt >/dev/null && rm test.txt && ls test.txt 2>/dev/null || echo test 4 successful
