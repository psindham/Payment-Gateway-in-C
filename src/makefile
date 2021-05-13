IDIR =../includes
CC=gcc
CFLAGS=-I$(IDIR)

ODIR=./obj

LDIR =../Library

LIBS=-lm

_CDEPS = CitiBank.h
CDEPS = $(patsubst %,$(IDIR)/%,$(_CDEPS))

_GDEPS = GateServer.h
GDEPS = $(patsubst %,$(IDIR)/%,$(_GDEPS))

_OBJ = CitiBankServer.o GatewayServer.o GatewayClient.o DepsCitiBank.o DepsGatewayServer.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

$(ODIR)/CitiBankServer.o:CitiBankServer.c 
	$(CC) -o $@ -c $< $(CFLAGS)

$(ODIR)/GatewayServer.o:GatewayServer.c 
	$(CC) -o $@ -c $< $(CFLAGS)

$(ODIR)/GatewayClient.o:GatewayClient.c
	$(CC) -o $@ -c $< 

$(ODIR)/DepsCitiBank.o:DepsCitiBank.c
	$(CC) -o $@ -c $< $(CFLAGS)

$(ODIR)/DepsGatewayServer.o:DepsGatewayServer.c
	$(CC) -o $@ -c $< $(CFLAGS)

$(ODIR)/ClientFD.o: ClientFD.c
	$(CC) -c -fPIC -o $@ $< $(CFLAGS)

$(ODIR)/ListenFD.o: ListenFD.c
	$(CC) -c -fPIC -o $@ $< $(CFLAGS)


_LIBOBJClient = ClientFD.o
LIBOBJClient = $(patsubst %,$(ODIR)/%,$(_LIBOBJClient))

$(LDIR)/libClient.so:$(LIBOBJClient)
	$(CC) -shared -o $@ $^

_LIBOBJListen = ListenFD.o
LIBOBJListen = $(patsubst %,$(ODIR)/%,$(_LIBOBJListen))

$(LDIR)/libListen.so:$(LIBOBJListen)
	$(CC) -shared -o $@ $^


_APPOBJGateServer = GatewayServer.o
APPOBJGateServer = $(patsubst %,$(ODIR)/%,$(_APPOBJGateServer))

_APPOBJGateClient = GatewayClient.o
APPOBJGateClient = $(patsubst %,$(ODIR)/%,$(_APPOBJGateClient))

_APPOBJCitiServer = CitiBankServer.o
APPOBJCitiServer = $(patsubst %,$(ODIR)/%,$(_APPOBJCitiServer))

CitiBankServer.out: $(APPOBJCitiServer) $(ODIR)/DepsCitiBank.o $(LDIR)/libListen.so 
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS) -pthread

GatewayServer.out: $(APPOBJGateServer) $(ODIR)/DepsGatewayServer.o $(LDIR)/libListen.so $(LDIR)/libClient.so 
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS) -pthread

GatewayClient.out: $(APPOBJGateClient)  $(LDIR)/libClient.so 
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS) 


.PHONY: clean
clean: 
	rm -f $(ODIR)/*.o *~ core $(INCDIR)/*~
	rm -f $(LDIR)/libClient.so $(LDIR)/libListen.so CitiBankServer.out GatewayServer.out GatewayClient.out

.PHONY: all
all:$(LDIR)/libClient.so $(LDIR)/libListen.so CitiBankServer.out GatewayServer.out GatewayClient.out 




