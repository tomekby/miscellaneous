@echo off
echo Generowanie rachunkow...
set path=c:\tools\python3;c:\tools\python3\scripts;C:\tools\wkhtmltopdf\bin;%path%
rem start bota
scrapy runspider invoice_bot.py -L ERROR

echo Rachunki wygenerowane
pause