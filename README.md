# SO-P1
##Rozszerzony problem producenta i konsumenta

###Podstawowy problem

W tym problemie występują dwa rodzaje procesów: producent i konsument, którzy dzielą wspólny bufor.
Producent tworzy produkty umieszczając je za każdym razem w buforze, a konsument pobiera produkty z bufora (pojedynczo). Producent nie może dodawać nowych produktów do bufora, gdy bufor jest pełny, a konsument nie może pobierać produktów, gdy bufor jest pusty. Tylko jeden agent może mieć dostęp do bufora w danej chwili.

###Rozszerzenie problemu

Mamy trzech producentów (kucharzy). Każdy producent wytwarza inny produkt i dodaje go do odpowiedniego bufora. Są trzy bufory o skończonych rozmiarach, każdy bufor przechowuje inny rodzaj produktów niż pozostałe bufory. Zestawy zamawiane przez klientów składają się zawsze z kompletu tych trzech produktów. Oprócz tego jest dodatkowy bufor, w którym znajdują się zamówienia składane przez klientów (co pewien ustalony czas tworzony jest klient o losowym adresie, który chce zamówić losową liczbę zestawów). Gdy jedna osoba ma dostęp do bufora zamówień, bufor jest zablokowany dla pozostałych (klientów i dostawców). Gdy bufor jest pełny, jest zablokowany dla klientów (nie można złożyć więcej zamówień, trzeba poczekać aż któryś z dostawców pobierze zamówienia). Dostawca, gdy uzyska dostęp do bufora, pobiera z niego tyle zamówień, ile jest w stanie obsłużyć. Jeśli liczba zamówień w buforze jest mniejsza od pojemności samochodu dostawczego, to dostawca pobiera wszystkie zamówienia z bufora. Po zebraniu zamówień dostawca kompletuje zestawy. Gdy dostawca skompletuje ilość zestawów wystarczającą do obsługi zebranych zamówień, rozwozi zestawy do klientów i wraca. Proces klienta po otrzymaniu ostatniego z zamówionych zestawów kończy działanie.
