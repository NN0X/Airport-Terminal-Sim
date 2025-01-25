# Samolot z bagażami

## Opis projektu

W terminalu pasażerskim czekają osoby z bagażem podręcznym i próbują wejść do samolotu, który
może pomieścić P pasażerów, dla których ustalony dopuszczalny ciężar bagażu podręcznego wynosi
Md.
Z uwagi na wymogi bezpieczeństwa ustalono następujące zasady:
• Najpierw odbywa się odprawa biletowo-bagażowa. Pasażer, którego bagaż waży więcej niż
dopuszczalny limit M, zostaje wycofany do terminala;
• Wejście na pokład samolotu możliwe będzie tylko po przejściu drobiazgowej kontroli, mającej
zapobiec wnoszeniu przedmiotów niebezpiecznych.
• Kontrola przy wejściu jest przeprowadzana równolegle na 3 stanowiskach, na każdym z nich
mogą znajdować się równocześnie maksymalnie 2 osoby.
• Jeśli kontrolowana jest więcej niż 1 osoba równocześnie na stanowisku, to należy
zagwarantować by były to osoby tej samej płci.
7
• Pasażer oczekujący na kontrolę może przepuścić w kolejce maksymalnie 3 innych
pasażerów. Dłuższe czekanie wywołuje jego frustrację i agresywne zachowanie, którego
należy unikać za wszelką cenę.
• Po przejściu przez kontrolę bezpieczeństwa pasażerowie oczekują na wejście na pokład
samolotu w tzw. holu odlotów. Wejście do samolotu odbywa się za pomocą tzw. gates, czyli
wyjść. Do każdego lotu przyporządkowane jest jedno wyjście, przy którym należy oczekiwać
na wejście do samolotu.
• W odpowiednim czasie obsługa lotniska (dyspozytor) poinformuje o gotowości przyjęcia
pasażerów na pokład samolotu.
• Pasażerowie są wpuszczani na pokład samolotu poprzez tzw. schody pasażerskie o
pojemności K (K<N).
• Istnieje pewna liczba osób (VIP) posiadająca bilety, które umożliwiają wejście na pokład
samolotu z pominięciem kolejki oczekujących (ale nie mogą ominąć schodów pasażerskich).
Samolot co określoną ilość czasu T1 (np.: pół godziny) wyrusza w podróż. Przed rozpoczęciem
kołowania kapitan samolotu musi dopilnować aby na schodach pasażerskich nie było żadnego
wchodzącego pasażera. Jednocześnie musi dopilnować by liczba pasażerów w samolocie nie
przekroczyła N. Ponadto samolot może odlecieć przed czasem T1 w momencie otrzymania polecenia
(sygnał1) od dyspozytora z wieży kontroli lotów.
Po odlocie samolotu na jego miejsce pojawia się natychmiast (jeżeli jest dostępny) nowy samolot o
takiej samej pojemności P jak poprzedni, ale z innym dopuszczalnym ciężarem bagażu podręcznego
Mdi. Łączna liczba samolotów wynosi N, każdy o pojemności P.
Samoloty przewożą pasażerów do miejsca docelowego i po czasie Ti wracają na lotnisko. Po
otrzymaniu od dyspozytora polecenia (sygnał 2) pasażerowie nie mogą wsiąść do żadnego samolotu
- nie mogą wejść na odprawę biletowo-bagażową. Samoloty kończą pracę po przewiezieniu
wszystkich pasażerów.

## Budowa programu

Zawarty w repozytorium Makefile używa clang++ do kompilacji programu. W przypadku potrzeby użycia innego kompilatora, należy zmienić zmienną CXX w pliku Makefile na odpowiednią wartość. Program działa tylko na systemach Linuxowych.

## Uruchomienie programu

```plaintext
./build/main <liczba pasażerów> <liczba samolotów>
```

## Zmienne

Wszystkie stałe są zdefiniowane w pliku [config.h](src/config.h).

## Problematyka

W związku z potencjalnie dużą ilością samolotów oraz pasażerów używanie sygnałów do komunikacji pomiędzy procesami jest praktycznie niemożliwe. W związku z tym ilość semaforów jest stosunkowo duża. Szczególnie widoczne jest to w przypadku komunikacji security control -
passenger, gdzie selector używa grupy N semaforów, gdzie N to ilość pasażerów co powoduje ograniczenie ilości pasażerów do maksymalnej ilości semaforów w grupie. [Odnośnik do kodu](src/secControl.cpp#L187)

## Użyte funkcje systemowe

- [unlink](src/main.cpp#L218)
- [fork](src/utils.cpp#L142)
- [exit](src/passenger.cpp#L75)
- [prctl](src/utils.cpp#L155)
- [pthread_create](src/secControl.cpp#L338)
- [kill](src/dispatcher.cpp#L60)
- [semop](src/utils.cpp#L254)
- [semctl](src/utils.cpp#L271)
- [mkfifo](src/secControl.cpp#L290)
- [open](src/utils.cpp#L189)

## Testy

N oraz M to dowolne unsigned int większe od 0  i mniejsze od 30000.

### Test 1

1 pasażer, 30000 samolotów

```plaintext
./build/main 1 30000
```

### Test 2

30000 pasażerów, 1 samolot

```plaintext
./build/main 30000 1
```

Działa ale wolno.

### Test 3

30000 pasażerów, 30000 samolotów

```plaintext
./build/main 30000 30000
```

Działa ale wolno.

### Test 4

pkill w trakcie działania programu

```plaintext
./build/main N M &
pkill main
```

Wszystkie procesy się kończą. Program zwraca błędy semaforów. Uważam, że jest to poprawne zachowanie w tym przypadku.

### Test 5

Pisanie do FIFO w trakcie działania programu

```plaintext
./build/main N M &
echo "test" > baggageControl
```

Nic się nie dzieje. Program działa poprawnie.

### Test 6

Odczyt z FIFO w trakcie działania programu

```plaintext
./build/main N M &
cat baggageControl
```

Nic się nie dzieje. Program działa poprawnie.

## Komentarz

W celu uniknięcia dużej ilości plików binarnych, program używa funkcji prctl do ustawienia nazwy procesu. Nie ma funkcjonalnej różnicy pomiędzy tą implementacją, a użyciem exec.
