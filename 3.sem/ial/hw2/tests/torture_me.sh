#!/bin/sh
#
#předem bych chtěl podotknout, že tyto testy byly přebrány z FIT serveru a 
#fitušky z loňských let a proto dík patří někomu jinému než mě :) já jsem je 
#jenom složil dohromady. Taky bych chtěl říct, že nepřebírám žádnou odpovědnost
#za správné výsledky ani málo bodů ve WISu.
#
#
#POKUD NAJDETE NĚJAKÉ BUGY NEBO UDĚLÁTE LEPŠÍ TESTY, TAK MI NAPIŠTE - BUDOU SE HODIT :)
#
#
#sada testů pro první domácí úkol do IALu
#soubor torture_me.sh musí být umístěn v adresáři, který obsahuje adresáře
#c201, c203, c206, které obsahují testovací soubory, správné výstupy, soubory  .c a .h
#a soubor Makefile
#
#
#skript postupně projede adresáře, pomocí make vytvoří spustitelné programy,
#programy spustí a jejich výstupy uloží do souborů .txt
#výstupy porovná s refenčními a vytvoří jejich rozdíl, který uloží do 
#příslušného souboru diff*.txt (pokud je soubor nulové velikosti, program 
#funguje správně)
#
#při správném výstupu se také provede ten na memory leaks valgrindem a jeho 
#výsledek se vypíše do souboru valgrind.txt a na terminál
#pokud není výstup správný, nakonec vám program nabídne rozdíly souborů vypsat 
#na terminál
#
#
# 

for t in c016 c401 c402
do
  echo "################################################################################"
  echo "Running tests for "$t":"                                                       
  cd $t
    test1=$t-test
    test2=$t-advanced-test
    rm $test1 $test2 2>/dev/null >/dev/null    
    make
    chmod +x $test1
    chmod +x $test2
################### standard test #############################################
    echo -n "   standard test:"
    ./$test1 > $test1-result.txt
    diff $test1.output $test1-result.txt >diff-standard.txt
    if [ -s diff-standard.txt ]
      then  echo "      failed"
            echo "Do you want to check differences (Y/N)?"
            read x
            if [ "$x" = "Y" -o "$x" = "y" ]
              then
                cat diff-standard.txt
            fi
      else echo "      success"
          valgrind --log-file=valgrind.txt ./$test1 >/dev/null
          echo "     ---------------------------------------------------------------VALGRIND OUT"
          cat valgrind.txt | sed 's/^.*[=]/     |/g' | grep bytes 
          echo "     ---------------------------------------------------------------VALGRIND OUT"
    fi
################### advanced test #############################################
    echo -n "   advanced test:"
    ./$test2 > $test2-result.txt
    diff $test2.output $test2-result.txt >diff-advanced.txt
    if [ -s diff-advanced.txt ]
      then  echo "      failed"
            echo "Do you want to check differences (Y/N)?"
            read x
            if [ "$x" = "Y" -o "$x" = "y" ]
              then
                cat diff-advanced.txt
            fi
      else echo "      success"
          valgrind --log-file=valgrind.txt ./$test2 >/dev/null
          echo "     ---------------------------------------------------------------VALGRIND OUT"
          cat valgrind.txt | sed 's/^.*[=]/     |/g' | grep bytes 
          echo "     ---------------------------------------------------------------VALGRIND OUT"    
    fi
    
  cd ..
  echo ""
done
echo "Do you want to remove temporary files (Y/N)?"
read x
if [ "$x" = "Y" -o "$x" = "y" ]
  then
    rm -f c016/*.txt c401/*.txt c402/*.txt
    rm -f c016/c016-test c016/c016-advanced-test
    rm -f c401/c401-test c401/c401-advanced-test
    rm -f c402/c402-test c402/c402-advanced-test
fi
############################## END ############################################
