Zależności:
- pywin32               [https://sourceforge.net/projects/pywin32/files/pywin32/]
- wkhtmltopdf           [http://wkhtmltopdf.org/downloads.html]
- Python 3.5+           [https://www.python.org/downloads/]
    - BeautifulSoup 4   [python -m pip install beautifulsoup4]
    - requests          [python -m pip install requests]
    - scrapy            [python -m pip install scrapy]

Uruchamianie:
- skrypt start.bat
    - zakłada, że python jest zainstalowany w c:\tools\python3 a scrapy jest zainstalowany globalnie
    - zakłada, że wkhtmltopdf jest zainstalowany w C:\tools\wkhtmltopdf  
    jeśli ww. ścieżki są inne, należy je zmienić w ustawianiu zmiennej %path%

Konfiguracja:
- jira-timesheet
    - ustawienia jiry, nie trzeba nic zmieniać
    - opcjonalnie:
        - month_from - miesiąc w którym zaczął się okres rozliczeniowy, jeśli nie ma, brany jest poprzedni miesiąc
- jira-login
    - dane do logowania do jiry - login i hasło
- invoice-data
    - dane do podstawienia na szablonie rachunku
    - nie są walidowane ani filtrowane, więc wszystkie wartości jakie tam są zostaną podstawione za odpowiadające pola w szablonie
- calculator
    - dane konfiguracyjne liczenia wartości do rachunku
        - wage: wynagrodzenie za 1h pracy NETTO; ta wartość * ilość godzin daje płacę netto za okres rozliczeniowy
        - amount-on-50%: część wynagrodzenia jaka jest rozliczana z przekazaniem praw autorskich
        - amount-on-20%: część wynagrodzenia jaka jest rozliczana bez ww.
    -techniczne:
        - default: domyślna klasa odpowiedzialna za przeliczanie wartości
        - fallback: klasa "zapasowa", używana jeśli default nie działa (jeśli domyślny kalkulator z internetu nie działa, obliczanie odbywa się offline)
        - web-bot: jeśli używana jest klasa WebCalculator, jest to nazwa używanego bota zbierającego dane z internetu
- wfirma.pl
    - dane dla kalkulatora ze strony wfirma.pl
    
Szablon
- Plik HTML w katalogu data zawierający szablon generowanego rachunku
