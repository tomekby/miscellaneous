@echo off
echo Generowanie rachunkow...
set path=c:\tools\python3;c:\tools\python3\scripts;C:\tools\wkhtmltopdf\bin;%path%
python manual.py %1

echo Rachunki wygenerowane
pause