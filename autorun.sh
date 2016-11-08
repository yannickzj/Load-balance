T=10
B=1024
P=6
C=15
P_t=8
R_s=10
C_t1=30
C_t2=5
p_i=0.4

make clean
make
for ((i=0; i<5; i++))
do
	Pi=$((P+i))
	echo P=$Pi
	echo multi-thread program is running.
	./bin/server_shm $T $B $Pi $C $P_t $R_s $C_t1 $C_t2 $p_i
	echo multi-process program is running.
	./bin/server_msq $T $B $Pi $C $P_t $R_s $C_t1 $C_t2 $p_i 
done

