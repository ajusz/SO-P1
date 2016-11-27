#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <vector>
#include <semaphore.h>
#include <iostream>
#include <sstream>

std::vector < pthread_t > producenci, klienci, dostawcy;	//wektory zawierające wątki producentów, klientów i dostawców
int pojemnosc;												//pojemność samochodów dostawczych
sem_t print;												//semafor synchronizujący wypisywanie na wyjście
bool wypisz_przy_tworzeniu;									//czy wypisywać informację o utworzeniu wątku
bool wypisuj_konce;											//czy wypisywać informację o zakończeniu wątku

std::string produkty[3] = {"frytki", "nuggetsy", "napój"};

template <class T> 
class Bufor
{
	T *bufor;
	int size;		//rozmiar bufora
	int pierwszy;	//indeks pierwszego elementu
	int ile;		//liczba elementów w buforze
	sem_t niepelny;	//semafor blokujący dodawanie elementów do pełnego bufora
	sem_t niepusty;	//semafor blokujący branie elementów z pustego bufora
	sem_t sekcja;
	bool end;
	int licznik;	//licznik wszystkich elementów dodawanych do bufora
	
	public:
	
	int max;

	~Bufor()
	{
		delete []bufor; 
	}
	
	void init(int n)
	{
		size = n;
		bufor = new T [size];
		pierwszy = 0;
		ile = 0;
		sem_init(&niepelny, 0, size);
		sem_init(&niepusty, 0, 0);
		sem_init(&sekcja, 0, 1);
		end = false;
		licznik = 0;
	}
	
	bool dodaj(T element, std::string s = NULL)
	{
		if(end)
		{
			return false;
		}
		sem_wait(&niepelny);
		sem_wait(&sekcja);
		if(end)
		{
			sem_post(&sekcja);
			sem_post(&niepelny);
			return false;
		}
		int gdzie = (pierwszy + ile)%size;	//pierwszego wolne miejsce w buforze
		bufor[gdzie] = element;
		ile++;
		licznik++;
		if(licznik == max)
			end = true;
		sem_wait(&print);
		std::cout<<s;						//wypisywanie komunikatu przekazanego przez wątek
		sem_post(&print);
		sem_post(&sekcja);
		sem_post(&niepusty);
		return true;
	}
	
	bool wez(T &res)
	{
		if(!end)
			sem_wait(&niepusty);			//jeżeli limit zamówień został wyczerpany, nie czekamy aż bufor przestanie być pusty
		sem_wait(&sekcja);
		if(ile==0 && end)
		{
			sem_post(&sekcja);
			return false;
		}
		ile--;
		res = bufor[pierwszy];
		pierwszy = (pierwszy+1)%size;	
		sem_post(&sekcja);
		sem_post(&niepelny);
		return true;
	}
};

struct zestaw
{
	int p1;
	int p2; 
	int p3;
};

struct zamowienie
{
	Bufor <zestaw> *bufor_klienta;	//wskaźnik na bufor klienta, do którego dostawca ma dostarczyć gotowy zestaw
	int nr_klienta;
	int nr_zam;
};

struct Klient
{
	Bufor <zestaw> *bufor_klienta;	//każdy klient ma swój bufor, do którego mają zostać dostarczone zamówione zestawy
	int nr_klienta;
	int ile_zam;
	
	Klient(int nr, int ile)
	{   nr_klienta = nr;
		ile_zam = ile;
		bufor_klienta=new Bufor <zestaw>();
		bufor_klienta->init(ile);	//rozmiar bufora to liczba złożonych przez danego klienta zamówień
	}
};

Bufor <int> buf[3];			//bufory producentów
Bufor <zamowienie> buf4;	//bufor zamówień


void* producent(void *arg)
{
	long i = (long)arg;
	for(int k=0; k<buf4.max; k++)	//producent produkuje aż do wyczerpania limitu zamówień na dzień
	{
		int p = rand()%100;
		std::stringstream s;
		s <<"Producent nr "<<i<<" wyprodukował "<< produkty[i]<<" nr "<<p<<std::endl;
		buf[i].dodaj(p, s.str());
		sleep(1);
	}
	if(wypisuj_konce)
	{
		sem_wait(&print);
		std::cout<<"Producent nr "<<i<<" kończy pracę."<<std::endl;
		sem_post(&print);
	}
	return NULL;
}

void* klient(void*arg)
{
	Klient *ik = (Klient *)arg;
	int ile = 0;							//licznik zamówień, które zostały przyjęte
	for(int i=0; i<ik->ile_zam; i++)		//klient składa tyle zamówień, ile zostało określone przy tworzeniu klienta
	{
		zamowienie zam;
		zam.nr_klienta = ik->nr_klienta;
		zam.bufor_klienta = ik->bufor_klienta;
		zam.nr_zam = i;
		std::stringstream s;
		s<<"Klient nr "<<ik->nr_klienta <<" złożył zamówienie nr "<< zam.nr_zam<<std::endl;
		if(buf4.dodaj(zam, s.str()))		//jeśli limit zamówień nie został wyczerpany, to zamówienie zostaje przyjęte
			ile++;
		else 								//jeśli limit zamówień został wyczerpany, to kolejne zamówienia zostają odrzucone
		{
			sem_wait(&print);
			std::cout<<"Klient nr "<<ik->nr_klienta <<" nie może więcej zamówić, ponieważ limit zamówień na dzień dzisiejszy został wyczerpany."<<std::endl; 
			sem_post(&print);
			break;
		}
	}
	
	for(int i=0; i<ile; i++)				//klient odbiera tyle zamówień, ile zostało przyjętych
	{
		zestaw z;
		ik->bufor_klienta->wez(z);
		
	}
	if(wypisuj_konce)
	{
		sem_wait(&print);
		std::cout<<"Klient nr "<<ik->nr_klienta<<" otrzymał wszystkie zamówienia."<<std::endl;
		sem_post(&print);
	}
	delete ik;
	return NULL;
}

void* dostawca(void *i)
{
	long nd = (long) i;						//nr dostawcy
	std::vector <zamowienie> zamowienia;	//wektor zamówień zebranych przez dostawcę z bufora
	std::vector <zestaw> zestawy;			//wektor skompletowanych zestawów
	
	while(1){
		int ile=0;							//licznik zebranych zamówień
		for(int j=0; j<pojemnosc; j++)
		{
			zamowienie zam;
			if(buf4.wez(zam))
			{
				ile++;
				zamowienia.push_back(zam);	//dodanie zamówienia do wektora zamówień
			}
			else
				break;
				
		}
		for(int j=0; j<ile; j++)			//kompletowanie zestawów
		{
			zestaw z;
			buf[0].wez(z.p1);
			buf[1].wez(z.p2);
			buf[2].wez(z.p3);
			zestawy.push_back(z);			//dodanie skompletowanego zestawu do wektora zestawów
		}	
		for(int j=0; j<ile; j++)
		{
			std::stringstream s;
			s<<"Dostawca nr "<<nd<<" dostarczył zestaw:"<<" frytki nr "<<zestawy[j].p1<<" nuggetsy nr "<<zestawy[j].p2<<" napój nr "<<zestawy[j].p3<<" klientowi nr "<<zamowienia[j].nr_klienta<<std::endl;
			zamowienia[j].bufor_klienta->dodaj(zestawy[j], s.str());	//dostarczenie j-tego zestawu klientowi z j-tego zamówienia
			sleep(1);
		}	
		for(int j=0; j<ile; j++)			//opróżnienie wektora zamówień i wektora zestawów (po dostarczeniu wszystkich zestawów klientom)
		{
			zamowienia.pop_back();
			zestawy.pop_back();
		}	
		if (ile < pojemnosc)				//jeśli dostawca nie wykorzystał całej pojemności samochodu dostawczego, to znaczy, że więcej zamówień już nie ma
			break;
	}
	if(wypisuj_konce)
	{
		sem_wait(&print);
		std::cout<<"Dostawca nr "<<nd<<" zakończył pracę."<<std::endl;
		sem_post(&print);
	}
	return NULL;
}

void* tworz_klientow(void *arg)
{
	long nk = (long)arg;
	int error;

	for(long i=0; i<nk; i++)
	{	

		error = 0;
		pthread_t k;
		error = pthread_create(&k, NULL, klient, (void *) new Klient(i,rand()%10+1));	//tworzenie i-tego klienta, z losową liczbą zamówień od 1 do 10
		if (error)
		{
			sem_wait(&print);
			printf("BŁĄÐ (przy tworzeniu wątku): %s\n", strerror(error));
			sem_post(&print);
			exit(-1);
		}
		klienci.push_back(k);	//dodanie nowo utworzonego klienta do wektora klientów
		if(wypisz_przy_tworzeniu)
		{
			sem_wait(&print);
			std::cout<<"Utworzono klienta nr "<<i<<std::endl;
			sem_post(&print);
		}
		sleep(1);
	}
	return NULL;
}

void tworz_watki(int nd)
{
	int error1 = 0;
	int error2 = 0;
	
	for(long i=0; i<3; i++)
	{
		pthread_t p;
		error1 = pthread_create(&p, NULL, producent, (void *)i);	//tworzenie i-tego producenta
		if (error1)
		{
			sem_wait(&print);
			printf("BŁĄÐ (przy tworzeniu wątku): %s\n", strerror(error1));
			sem_post(&print);
			exit(-1);
		}
		producenci.push_back(p); //dodanie nowo utworzonego producenta do wektora producentów
		if(wypisz_przy_tworzeniu)
		{
			sem_wait(&print);
			std::cout<<"Utworzono producenta nr "<<i<<std::endl;
			sem_post(&print);
		}
	}
	
	for(long i=0; i<nd; i++)
	{
		pthread_t d;
		error2 = pthread_create(&d, NULL, dostawca, (void *)i);	//tworzenie i-tego dostawcy
		if (error2)
		{
			sem_wait(&print);
			printf("BŁĄÐ (przy tworzeniu wątku): %s\n", strerror(error2));
			sem_post(&print);
			exit(-1);
		}
		dostawcy.push_back(d);	//dodanie nowo utworzonego dostawcy do wektora dostawców
		if(wypisz_przy_tworzeniu)
		{
			sem_wait(&print);
			std::cout<<"Utworzono dostawcę nr "<<i<<std::endl;
			sem_post(&print);
		}
	}	
}

int main ()
{
	buf[0].init(5);					//bufor producenta nr 0
	buf[1].init(5);					//bufor producenta nr 1
	buf[2].init(5);					//bufor producenta nr 2
	buf4.init(10);					//bufor zamówień
	int nd = 5;						//liczba dostawców
	long nk = 10;					//liczba klientów
	pojemnosc = 5;					//pojemność samochodów dostawczych
	wypisz_przy_tworzeniu = true;	//czy wypisywać informację o utworzeniu wątku
	wypisuj_konce = true;			//czy wypisywać informację o zakończeniu wątku
	
	buf4.max = 4*nk;				//maksymalna liczba zamówień w ciągu dnia
	sem_init(&print, 0, 1);			//semafor synchronizujący wypisywanie na wyjście
	int error = 0;
	pthread_t tworca_klientow;
	error = pthread_create(&tworca_klientow, NULL, tworz_klientow, (void *)nk);
	if (error)
	{
		sem_wait(&print);
		printf("BŁĄÐ (przy tworzeniu wątku): %s\n", strerror(error));
		sem_post(&print);
		exit(-1);
	}
	tworz_watki(nd);

	for(long i=0; i<nk; i++)
	{
		error = pthread_join(klienci[i], NULL);
		if(error)
		{
			sem_wait(&print);
			printf("BŁĄD (przy oczekiwaniu na zakończenie wątków): %s\n", strerror(error));
			sem_post(&print);
			exit(-1);
		}
	}
	for(int j=0; j<nd; j++)
	{
		error = pthread_join(dostawcy[j], NULL);
		if(error)
		{
			sem_wait(&print);
			printf("BŁĄD (przy oczekiwaniu na zakończenie wątków): %s\n", strerror(error));
			sem_post(&print);
			exit(-1);
		}
	}
	for(int k=0; k<3; k++)
	{
		error = pthread_join(producenci[k], NULL);
		if(error)
		{
			sem_wait(&print);
			printf("BŁĄD (przy oczekiwaniu na zakończenie wątków): %s\n", strerror(error));
			sem_post(&print);
			exit(-1);
		}
	}
	pthread_exit(NULL);
	return 0;
}
