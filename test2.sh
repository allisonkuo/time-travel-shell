#! /bin/sh

echo a && echo b && (echo c || echo NO) && cat a.txt > aInit.txt && echo c.txt

cat b.txt > a.txt && echo c.txt && echo c.txt

cat a.txt > aNew.txt && echo c.txt

sort a.txt > aSorted.txt && (ls none.txt || echo YES) && echo YES2 && echo c.txt

echo LastLine > c.txt && cat c.txt



