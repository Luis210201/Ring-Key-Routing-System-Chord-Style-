# RCI, última atualização 14/04/2022
#
# To compile prog:
#    make
#
#----------------------------------------------------------------------


bin: *.c
	cc ring.c -Wall -o ring
	chmod +x ring
