��   �   0   h  �   �     �     �  �        �     7  �  �  '  -   �  F        e     }  7   �  �   �  �   J  �   >  �  �  H   �  E   �  �       �  >   �  9     �   ?  �   �  �   W  �   �  �   �  �   >  l   �     i     �     �     �     �     �     	     #  z   =     �     �     �  .   �  6   -     d     w     �     �  !   �  !   �  '   �           2      R   *   q   /   �   %   �      �   /   !  ,   4!     a!  4   w!     �!     �!     �!     �!     "      4"      U"  h   v"  <   �"     #  :   /#  $   j#     �#  2   �#     �#  $   �#  /   $  I   O$     �$  3   �$  =   �$  d   %      �%  O   �%  .   �%  /   $&     T&  A   o&  )   �&     �&     �&  8   �&     6'     Q'  (   f'  I   �'  !   �'  '   �'  '   #(  9   K(     �(     �(  0   �(     �(  <   �(  -   )  @   D)  /   �)  7   �)  D   �)  &   2*  '   Y*     �*  %   �*     �*     �*  
   �*  
   �*  
   �*  
   �*  
   +  	   +  	   +  	    +  	   *+  	   4+  	   >+  	   H+  "   R+  *   u+     �+  A   �+  Q   �+  *   H,  @   s,  !   �,     �,  �  �,  �   �.  v  �/  �  3  .   �4  P   5     ^5     x5  K   �5  �   �5    n6  �   �7  �  I8  F   :  E   `:  �   �:    �;  K   �<  F   �<  �   /=  �   �=  �   �>  �   )?  �   @  �   �@  p   �A     �A     B     6B  "   SB     vB     �B     �B     �B  �   �B     sC     �C     �C  +   �C  6   �C     D     =D     UD     ZD  '   vD  /   �D  ,   �D  ,   �D  #   (E  -   LE  .   zE  7   �E  ,   �E     F  9   "F  4   \F     �F  I   �F  %   �F  !   G     ?G     XG     uG  .   �G  )   �G  d   �G  H   SH     �H  S   �H  !   I     )I  >   HI     �I  ,   �I  5   �I  J   	J     TJ  7   jJ  B   �J  k   �J  '   QK  j   yK  5   �K  8   L  !   SL  G   uL  .   �L     �L     �L  H   M     _M     {M  ,   �M  Z   �M  $   N  +   ;N  +   gN  G   �N  #   �N     �N  H   O     LO  F   QO  �   �O  C   GP  )   �P  7   �P  `   �P  %   NQ  -   tQ     �Q  1   �Q     �Q     �Q  
   R  
   R  
   %R  
   0R  
   ;R  
   FR  
   QR  
   \R  
   gR  
   rR  
   }R  
   �R  9   �R  3   �R     S  W   S  V   oS  C   �S  A   
T  0   LT     }T     [   :           F       y   �           �          �   .              R   r      (          \             2   5       n           a       6   _   %       0   `       
   Z       *   X   �   v   ^          G      {      g            W       "           p      -                          u          !   b       >          I      h           )   z       O       L   ?           &   <       =   C       �       ]   q   c   D             �       9              +   x   k   i   ;   S   N   /      V   Y       j   J   $           H       B   A   l      o   8      m      U   t   s   4   w       	      K         �   f   1           �   '   E   T       3   d   |   }   ~       Q   #       e               M   ,   7              @   P         �T     �T  �  �  �     �T  2          �����T  0               �����T  /          ����'U  .               ���� 
  --delta[=OPTS]      Delta filter; valid OPTS (valid values; default):
                        dist=NUM   distance between bytes being subtracted
                                   from each other (1-256; 1) 
  --lzma1[=OPTS]      LZMA1 or LZMA2; OPTS is a comma-separated list of zero or
  --lzma2[=OPTS]      more of the following options (valid values; default):
                        preset=PRE reset options to a preset (0-9[e])
                        dict=NUM   dictionary size (4KiB - 1536MiB; 8MiB)
                        lc=NUM     number of literal context bits (0-4; 3)
                        lp=NUM     number of literal position bits (0-4; 0)
                        pb=NUM     number of position bits (0-4; 2)
                        mode=MODE  compression mode (fast, normal; normal)
                        nice=NUM   nice length of a match (2-273; 64)
                        mf=NAME    match finder (hc3, hc4, bt2, bt3, bt4; bt4)
                        depth=NUM  maximum search depth; 0=automatic (default) 
  --x86[=OPTS]        x86 BCJ filter (32-bit and 64-bit)
  --powerpc[=OPTS]    PowerPC BCJ filter (big endian only)
  --ia64[=OPTS]       IA-64 (Itanium) BCJ filter
  --arm[=OPTS]        ARM BCJ filter (little endian only)
  --armthumb[=OPTS]   ARM-Thumb BCJ filter (little endian only)
  --sparc[=OPTS]      SPARC BCJ filter
                      Valid OPTS for all BCJ filters:
                        start=NUM  start offset for conversions (default=0) 
 Basic file format and compression options:
 
 Custom filter chain for compression (alternative for using presets): 
 Operation modifiers:
 
 Other options:
 
With no FILE, or when FILE is -, read standard input.
       --info-memory   display the total amount of RAM and the currently active
                      memory usage limits, and exit       --memlimit-compress=LIMIT
      --memlimit-decompress=LIMIT
  -M, --memlimit=LIMIT
                      set memory usage limit for compression, decompression,
                      or both; LIMIT is in bytes, % of RAM, or 0 for defaults       --no-adjust     if compression settings exceed the memory usage limit,
                      give an error instead of adjusting the settings downwards       --no-sparse     do not create sparse files when decompressing
  -S, --suffix=.SUF   use the suffix `.SUF' on compressed files
      --files[=FILE]  read filenames to process from FILE; if FILE is
                      omitted, filenames are read from the standard input;
                      filenames must be terminated with the newline character
      --files0[=FILE] like --files but use the null character as terminator       --robot         use machine-parsable messages (useful for scripts)       CheckVal %*s Header  Flags        CompSize    MemUsage  Filters   -0 ... -9           compression preset; default is 6; take compressor *and*
                      decompressor memory usage into account before using 7-9!   -F, --format=FMT    file format to encode or decode; possible values are
                      `auto' (default), `xz', `lzma', and `raw'
  -C, --check=CHECK   integrity check type: `none' (use with caution),
                      `crc32', `crc64' (default), or `sha256'   -Q, --no-warn       make warnings not affect the exit status   -V, --version       display the version number and exit   -e, --extreme       try to improve compression ratio by using more CPU time;
                      does not affect decompressor memory requirements   -h, --help          display the short help (lists only the basic options)
  -H, --long-help     display this long help and exit   -h, --help          display this short help and exit
  -H, --long-help     display the long help (lists also the advanced options)   -k, --keep          keep (don't delete) input files
  -f, --force         force overwrite of output file and (de)compress links
  -c, --stdout        write to standard output and don't delete input files   -q, --quiet         suppress warnings; specify twice to suppress errors too
  -v, --verbose       be verbose; specify twice for even more verbose   -z, --compress      force compression
  -d, --decompress    force decompression
  -t, --test          test compressed file integrity
  -l, --list          list information about .xz files   Blocks:
    Stream     Block      CompOffset    UncompOffset       TotalSize      UncompSize  Ratio  Check   Blocks:             %s
   Check:              %s
   Compressed size:    %s
   Memory needed:      %s MiB
   Number of files:    %s
   Ratio:              %s
   Sizes in headers:   %s
   Stream padding:     %s
   Streams:
    Stream    Blocks      CompOffset    UncompOffset        CompSize      UncompSize  Ratio  Check      Padding   Streams:            %s
   Uncompressed size:  %s
  Operation mode:
 %s MiB of memory is required. The limit is %s. %s MiB of memory is required. The limiter is disabled. %s file
 %s files
 %s home page: <%s>
 %s:  %s: Cannot remove: %s %s: Cannot set the file group: %s %s: Cannot set the file owner: %s %s: Cannot set the file permissions: %s %s: Closing the file failed: %s %s: Error reading filenames: %s %s: Error seeking the file: %s %s: File already has `%s' suffix, skipping %s: File has setuid or setgid bit set, skipping %s: File has sticky bit set, skipping %s: File is empty %s: File seems to have been moved, not removing %s: Filename has an unknown suffix, skipping %s: Filter chain: %s
 %s: Input file has more than one hard link, skipping %s: Invalid filename suffix %s: Invalid multiplier suffix %s: Invalid option name %s: Invalid option value %s: Is a directory, skipping %s: Is a symbolic link, skipping %s: Not a regular file, skipping %s: Null character found when reading filenames; maybe you meant to use `--files0' instead of `--files'? %s: Options must be `name=value' pairs separated with commas %s: Read error: %s %s: Seeking failed when trying to create a sparse file: %s %s: Too small to be a valid .xz file %s: Unexpected end of file %s: Unexpected end of input when reading filenames %s: Unknown file format type %s: Unsupported integrity check type %s: Value is not a non-negative decimal integer %s: With --format=raw, --suffix=.SUF is required unless writing to stdout %s: Write error: %s --list does not support reading from standard input --list works only on .xz files (--format=xz or --format=auto) Adjusted LZMA%c dictionary size from %s MiB to %s MiB to not exceed the memory usage limit of %s MiB Cannot establish signal handlers Cannot read data from standard input when reading filenames from standard input Compressed data cannot be read from a terminal Compressed data cannot be written to a terminal Compressed data is corrupt Compression and decompression with --robot are not supported yet. Decompression will need %s MiB of memory. Disabled Empty filename, skipping Error restoring the O_APPEND flag to standard output: %s File format not recognized Internal error (bug) LZMA1 cannot be used with the .xz format Mandatory arguments to long options are mandatory for short options too.
 Maximum number of filters is four Memory usage limit for compression:     Memory usage limit for decompression:   Memory usage limit is too low for the given filter setup. Memory usage limit reached No No integrity check; not verifying file integrity None Only one file can be specified with `--files' or `--files0'. Report bugs to <%s> (in English or Finnish).
 Strms  Blocks   Compressed Uncompressed  Ratio  Check   Filename The .lzma format supports only the LZMA1 filter The environment variable %s contains too many arguments The exact options of the presets may vary between software versions. The sum of lc and lp must not exceed 4 Total amount of physical memory (RAM):  Totals: Try `%s --help' for more information. Unexpected end of input Unknown error Unknown-11 Unknown-12 Unknown-13 Unknown-14 Unknown-15 Unknown-2 Unknown-3 Unknown-5 Unknown-6 Unknown-7 Unknown-8 Unknown-9 Unsupported LZMA1/LZMA2 preset: %s Unsupported filter chain or filter options Unsupported options Unsupported type of integrity check; not verifying file integrity Usage: %s [OPTION]... [FILE]...
Compress or decompress FILEs in the .xz format.

 Using a preset in raw mode is discouraged. Valid suffixes are `KiB' (2^10), `MiB' (2^20), and `GiB' (2^30). Writing to standard output failed Yes Project-Id-Version: xz 5.0.2
Report-Msgid-Bugs-To: lasse.collin@tukaani.org
POT-Creation-Date: 2014-12-21 15:56+0200
PO-Revision-Date: 2012-05-29 18:15+0200
Last-Translator: Jakub Bogusz <qboosh@pld-linux.org>
Language-Team: Polish <translation-team-pl@lists.sourceforge.net>
Language: pl
MIME-Version: 1.0
Content-Type: text/plain; charset=UTF-8
Content-Transfer-Encoding: 8bit
Plural-Forms: nplurals=3; plural=n==1 ? 0 : n%10>=2 && n%10<=4 && (n%100<10 || n%100>=20) ? 1 : 2;
 
  --delta[=OPCJE]     Filtr delta; poprawne OPCJE (poprawne wart.; domyślne):
                        dist=ILE   odległość między bajtami odejmowanymi od
                                   siebie (1-256; 1) 
  --lzma1[=OPCJE]     LZMA1 lub LZMA2; OPCJE to oddzielona przecinkami lista
  --lzma2[=OPCJE]     zera lub więcej następujących opcji (w nawiasach wartości
                      poprawne; domyślne):
                        preset=PRE ustawienie opcji na predefiniowane (0-9[e])
                        dict=ILE   rozmiar słownika (4KiB - 1536MiB; 8MiB)
                        lc=ILE     liczba bitów kontekstu literału (0-4; 3)
                        lp=ILE     liczba bitów pozycji literału (0-4; 0)
                        pp=ILE     liczba bitów pozycji (0-4; 2)
                        mode=TRYB  tryb kompresji (fast, normal; normal)
                        nice=ILE   długość dopasowania (2-273; 64)
                        mf=NAZWA   dopasowywacz (hc3, hc4, bt2, bt3, bt4; bt4)
                        depth=ILE  maks. głębokość szukania; 0=auto (domyślne) 
  --x86[=OPCJE]       Filtr BCJ x86 (32-bitowy lub 64-bitowy)
  --powerpc[=OPCJE]   Filtr BCJ PowerPC (tylko big-endian)
  --ia64[=OPCJE]      Filtr BCJ IA-64 (Itanium)
  --arm[=OPCJE]       Filtr BCJ ARM (tylko little-endian)
  --armthumb[=OPCJE]  Filtr BCJ ARM-Thumb (tylko little-endian)
  --sparc[=OPCJE]     Filtr BCJ SPARC
                      Poprawne OPCJE dla wszystkich filtrów BCJ:
                        start=ILE  offset początku konwersji (domyślnie=0) 
 Podstawowe opcje formatu pliku i kompresji:
 
 Łańcuch własnych filtrów do kompresji (alternatywa do używania -0 .. -9): 
 Modyfikatory operacji:
 
 Inne opcje:
 
Jeśli nie podano PLIKU lub PLIK to -, czytane jest standardowe wejście.
       --info-memory   wyświetlenie całkowitej ilości pamięci RAM oraz aktualnie
                      aktywnych limitów pamięci i zakończenie pracy       --memlimit-compress=LIMIT
      --memlimit-decompress=LIMIT
  -M, --memlimit=LIMIT
                      ustawienie limitu użycia pamięci dla kompresji,
                      dekompresji lub obu; LIMIT jest w bajtach, % RAM lub 0
                      dla limitów domyślnych       --no-adjust     jeśli ustawienia kompresji przekraczają limit użycia
                      pamięci, zostanie zgłoszony błąd zamiast zmniejszania
                      ustawień       --no-sparse     nietworzenie plików rzadkich podczas dekompresji
  -S, --suffix=.ROZ   użycie rozszerzenia `.ROZ' dla plików skompresowanych
      --files[=PLIK]  odczyt nazw plików do przetworzenia z PLIKU; jeśli PLIK
                      nie został podany, nazwy są czytane ze standardowego
                      wejścia; muszą być zakończone znakiem nowej linii
      --files0[=PLIK] podobnie do --files, ale znakiem kończącym musi być NUL       --robot         komunikaty w formacie dla maszyny (do skryptów)    S.kontr. %*sNagłówek Flagi     Rozm. spak.  Uż.pamięci  Filtry   -0 ... -9           predefiniowane opcje kompresji; domyślna to 6; przed
                      użyciem wartości 7-9 należy wziąć pod uwagę wykorzystanie
                      pamięci przy kompresji *oraz* dekompresji!   -F, --format=FORM   format pliki do kodowania lub dekodowania; możliwe to
                      `auto' (domyślny), `xz', 'lzma' i `raw'
  -C, --check=TEST    typ kontroli spójności: `none' (ostrożnie!),
                      `crc32', `crc64' (domyślny) lub `sha256'   -Q, --no-warn       ostrzeżenia nie mają wpływu na status zakończenia   -V, --version       wyświetlenie informacji o wersji i zakończenie   -e, --extreme       próba poprawy współczynnika kompresji z użyciem większej
                      ilości czasu procesora; nie wpływa na wymagania
                      pamięciowe dekompresora   -h, --help          wyświetlenie krótkiego opisu (tylko podstawowe opcje)
  -H, --long-help     wyświetlenie tego długiego opisu i zakończenie   -h, --help          wyświetlenie tego krótkiego opisu i zakończenie
  -H, --long-help     wyświetlenie długiego opisu (także opcje zaawansowane)   -k, --keep          zachowanie (nieusuwanie) plików wejściowych
  -f, --force         nadpisywanie plików wyjściowych i (de)kompresja dowiązań
  -c, --stdout        zapis na standardowe wyjście, nieusuwanie plików wej.   -q, --quiet         pominięcie ostrzeżeń; dwukrotne podanie pomija też błędy
  -v, --verbose       więcej informacji; dwukrotne podanie to jeszcze więcej   -z, --compress      wymuszenie kompresji
  -d, --decompress    wymuszenie dekompresji
  -t, --test          sprawdzenie spójności plików skompresowanych
  -l, --list          wypisanie informacji o plikach .xz   Bloki:
  Strumień      Blok    Offset spak.    Offset rozp.  Rozm.całkowity      Rozm.rozp.  Wsp.   Kontrola   Bloki:                 %s
   Kontrola spójności:    %s
   Rozmiar spakowany:     %s
   Wymagana pamięć:       %s MiB
   Liczba plików:         %s
   Współczynnik:          %s
   Rozmiar w nagłówkach:  %s
   Wyrównanie strumienia: %s
   Strumienie:
  Strumień     Bloki    Offset spak.    Offset rozp.        Rozm.spak.    Rozm.rozp.  Wsp.   Kontrola   Wyrównanie   Strumienie:            %s
   Rozmiar rozpakowany:   %s
  Tryb pracy:
 Wymagane jest %s MiB pamięci. Limit to %s. Wymagane jest %s MiB pamięci. Limit jest wyłączony. %s plik
 %s pliki
 %s plików
 Strona domowa %s: <%s>
 %s:  %s: Nie można usunąć: %s %s: Nie można ustawić grupy pliku: %s %s: Nie można ustawić właściciela pliku: %s %s: Nie można ustawić uprawnień pliku: %s %s: Zamknięcie pliku nie powiodło się: %s %s: Błąd odczytu nazw plików: %s %s: Błąd podczas zmiany pozycji w pliku: %s %s: Plik już ma rozszerzenie `%s', pominięto %s: Plik ma ustawiony bit setuid lub setgid, pominięto %s: Plik ma ustawiony bit sticky, pominięto %s: Plik jest pusty %s: Plik wygląda na przeniesiony, nie zostanie usunięty %s: Nazwa pliku ma nieznane rozszerzenie, pominięto %s: Łańcuch filtrów: %s
 %s: Plik wejściowy ma więcej niż jedno dowiązanie zwykłe, pominięto %s: Błędne rozszerzenie nazwy pliku %s: Błędny przyrostek mnożnika %s: Błędna nazwa opcji %s: Błędna wartość opcji %s: Jest katalogiem, pominięto %s: Jest dowiązaniem symbolicznym, pominięto %s: Nie jest zwykłym plikiem, pominięto %s: Napotkano znak NUL podczas odczytu nazw plików; może miało być `--files0' zamiast `--files'? %s: Opcje muszą być parami `nazwa=wartość' rozdzielonymi przecinkami %s: Błąd odczytu: %s %s: Zmiana pozycji nie powiodła się podczas próby utworzenia pliku rzadkiego: %s %s: Za mały na poprawny plik .xz %s: Nieoczekiwany koniec pliku %s: Nieoczekiwany koniec wejścia podczas odczytu nazw plików %s: Nieznany typ formatu pliku %s: Nieobsługiwany typ kontroli spójności %s: Wartość nie jest nieujemną liczbą całkowitą %s: Przy --format=raw i zapisie do pliku wymagana jest opcja --suffix=.ROZ %s: Błąd zapisu: %s --list nie obsługuje odczytu ze standardowego wejścia --list działa tylko z plikami .xz (--format=xz lub --format=auto) Skorygowano rozmiar słownika LZMA%c z %s MiB do %s MiB aby nie przekroczyć limitu użycia pamięci %s MiB Nie można ustawić obsługi sygnałów Nie można odczytać danych ze standardowego wejścia przy czytaniu nazw plików ze standardowego wejścia Dane skompresowane nie mogą być czytane z terminala Dane skompresowane nie mogą być zapisywane na terminal Dane skompresowane są uszkodzone Kompresja i dekompresja z opcją --robot nie jest jeszcze obsługiwana. Dekompresja będzie wymagała %s MiB pamięci. Wyłączony Pusta nazwa pliku, pominięto Błąd podczas odtwarzania flagi O_APPEND dla standardowego wyjścia: %s Nie rozpoznany format pliku Błąd wewnętrzny LZMA1 nie może być używany z formatem .xz Argumenty obowiązkowe dla opcji długich są obowiązkowe również dla opcji
krótkich.
 Maksymalna liczba filtrów to cztery Limit użycia pamięci dla kompresji:       Limit użycia pamięci dla dekompresji:     Limit użycia pamięci jest zbyt mały dla podanej konfiguracji filtra. Osiągnięto limit użycia pamięci Nie Brak kontroli spójności; poprawność plików nie będzie weryfikowana Brak Wraz z opcją `--files' lub `--files0' można podać tylko jeden plik. Błędy prosimy zgłaszać na adres <%s>
(w języku angielskim lub fińskim).
Błędy w tłumaczeniu prosimy zgłaszać na adres
<translation-team-pl@lists.sourceforge.net>.
 Strum.  Bloki    Spakowany  Rozpakowany  Wsp.  Kontrola Nazwa pliku Format .lzma obsługuje tylko filtr LZMA1 Zmienna środowiskowa %s zawiera zbyt dużo argumentów Dokładne opcje ustawień predefiniowanych mogą różnić się między wersjami oprogramowania. Suma lc i lp nie może przekroczyć 4 Całkowita ilość pamięci fizycznej (RAM):  Sumarycznie: Polecenie `%s --help' pokaże więcej informacji. Nieoczekiwany koniec wejścia Nieznany błąd Nieznany11 Nieznany12 Nieznany13 Nieznany14 Nieznany15 Nieznany-2 Nieznany-3 Nieznany-5 Nieznany-6 Nieznany-7 Nieznany-8 Nieznany-9 Nieobsługiwane ustawienie predefiniowane LZMA1/LZMA2: %s Nieobsługiwany łańcuch filtrów lub opcje filtra Nieobsługiwane opcje Nieobsługiwany typ kontroli spójności; poprawność plików nie będzie weryfikowana Składnia: %s [OPCJA]... [PLIK]...
Kompresja lub dekompresja PLIKÓW w formacie .xz.

 Użycie ustawień predefiniowanych w trybie surowym jest odradzane. Poprawne przyrostki to `KiB' (2^10), `MiB' (2^20) i `GiB' (2^30). Zapis na standardowe wyjście nie powiódł się Tak PRIu32 PRIu64 The selected match finder requires at least nice=% Value of the option `%s' must be in the range [%, %] Wybrany dopasowywacz wymaga przynajmniej nice=% Wartość opcji `%s' musi być w przedziale [%, %] 