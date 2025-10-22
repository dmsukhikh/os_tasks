#!/usr/bin/bash

# Скрипт тестирования удаления из архива
# usage: ./scr
# ./scr clean - очистить файлы тестирования

names=(a b c d)

# Если мы хотим очистить файлы тестирования
echo "$1"
if [[ "$1" = "clean" ]]; then
    echo "cleaning..."
    rm -r template
    rm test.egl
    echo "done!"
    exit 0
fi;

if [[ (-d template) ]]; then
    rm -r template
    echo "dir template was cleaned"
fi;

if [[ (-e test.egl) ]]; then
    rm test.egl 
    echo "test.egl was cleaned"
fi;

mkdir template;
for name in ${names[@]}; do
    echo "this is ${name} file" > "${name}".txt;
    mv "${name}.txt" template
    $PWD/archiver test.egl -i $PWD/template/"$name".txt
    echo "file $name.txt was added"
done;

$PWD/archiver test.egl -s
echo "start removing..."
$PWD/archiver test.egl -r a.txt
$PWD/archiver test.egl -s
$PWD/archiver test.egl -r c.txt
$PWD/archiver test.egl -s

echo "revert changes and running under valgrind..."
$PWD/archiver test.egl -i $PWD/template/a.txt
$PWD/archiver test.egl -i $PWD/template/c.txt
valgrind $PWD/archiver test.egl -r b.txt d.txt
$PWD/archiver test.egl -s

echo "done!"

# ../archiver test.egl -i 

