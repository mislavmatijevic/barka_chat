za_barku:
	g++ -static chat_klijent.cpp -o chat_klijent -lpthread -l:libncursesw.a -l:libtinfo.a	
	g++ -static chat_server.cpp -o chat_server -lpthread
	scp -p2222 chat_klijent chat_server mmatijevi@barka.foi.hr:barka_chat/
