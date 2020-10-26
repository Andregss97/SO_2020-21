#!/bin/bash
echo Primeira demo do operador for
for i in 0 1 2
do
 echo $i
done

echo ------
echo Implementação equivalente usando expansão de comandos e o comando seq
echo Para compreender este exemplo, experimente executar seq 0 2 numa shell
echo Consulte o manual do comando seq man seq
for i in $(seq 0 2)
do
 echo $i
done
echo ------
echo O output do comando ls é expandido na variável de iteração f
for f in $(ls /usr/include/*.h)
do
 echo filename: $f
done
echo Neste exemplo vamos executar o programa Test passando-lhe
echo como input todos os ficheiros presentes na pasta inputs.
echo NOTA: Para que o script funcione, tem de ser executado
echo na mesma pasta onde se encontra o executável Test
for input in inputs/*.txt
do
 echo ==== ${input} ====
 ./tecnicofs <"$input"
done