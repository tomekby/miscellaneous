import codecs
from subprocess import call
import os
import json
import datetime
from datetime import date, timedelta
import sys
import calculator


# If key 'month_from' don't exist in data['jira-timesheet'] then we set last month in its place
# Configuration class
class Config:
    _data = None

    # Init config from file
    @staticmethod
    def init():
        with codecs.open('data/config.json', 'r', "utf-8") as config_file:
            Config._data = json.loads(config_file.read())
        # Set 'missing' data
        Config._data['invoice-data']['today_date'] = datetime.date.today().strftime('%d.%m.%Y')
        if 'month_from' not in Config._data['jira-timesheet']:
            prev = date.today().replace(day=1) - timedelta(days=1)
            Config._data['jira-timesheet']['month_from'] = prev.month

    # Get config section
    @staticmethod
    def get(what):
        return Config._data.get(what, {})

    # Get start/end time of interval
    # todo przetwarzanie daty po ludzku
    @staticmethod
    def get_date(what):
        # Start date
        if what == 'from':
            year_start = date.today().year - 1 if Config._data.get('jira-timesheet')['month_from'] == 12 else date.today().year
            return '%d-%d-24' % (year_start, Config.get('jira-timesheet')['month_from'])
        # End date
        month_end = Config.get('jira-timesheet')['month_from'] + 1
        return '%d-%d-23' % (date.today().year, month_end if month_end < 13 else 1)
    
    # Timesheet URI params
    @staticmethod
    def get_uri_params():
        return Config.get('jira-timesheet')['params'] \
            .replace('{date_from}', Config.get_date('from')) \
            .replace('{date_to}', Config.get_date('to'))

    # Get needed calculator class
    @staticmethod
    def get_calculator():
        return getattr(calculator, Config.get('calculator')['default'])

    # Get bot class used by web calculator
    @staticmethod
    def get_calculator_bot():
        return getattr(calculator, Config.get('calculator')['web-bot'])

    # Get fallback calculator class if the first one is not achievable
    @staticmethod
    def get_fallback_calculator():
        return getattr(calculator, Config.get('calculator')['fallback'])

# Load configuration
Config.init()


# Basic processor of crawler data
class BasicProcessor:
    _amount_on_20 = 0
    _amount_on_50 = 0

    def __init__(self, amount_on_50, amount_on_20):
        self._amount_on_20 = amount_on_20
        self._amount_on_50 = amount_on_50

    # 50% cost data
    def get_50_data(self, hours_amount):
        calc = Config.get_calculator()(self._amount_on_50 / 100 * hours_amount, Config.get('calculator')['wage'], 50)
        return {**self.get_data(calc), **{
            'what': 'z tytułu przeniesienia autorskich praw majątkowych do stworzonego przez Wykonawcę Oprogramowania'
        }}

    # 20% cost data
    def get_20_data(self, hours_amount):
        calc = Config.get_calculator()((self._amount_on_20 / 100) * hours_amount, Config.get('calculator')['wage'], 20)
        return {**self.get_data(calc), **{
            'what': 'z tytułu wdrożenia przez Wykonawcę Oprogramowania'
        }}

    # Data needed by the template
    @staticmethod
    def get_data(calc):
        num2string = NumberToString()
        return {
            'net': calc.get_net(),
            'gross': round(calc.get_gross(), 2),
            'cost': round(calc.get_cost(), 2),
            'tax_base': int(calc.get_tax_base() + 0.5),
            'tax': round(calc.get_tax()),
            'net_string': num2string.process(calc.get_net()),
            'percentage': '%d%%' % calc.get_cost_percentage(),
        }

    # Generating both pdf files
    def generate_pdf(self, hours_amount):
        hours_amount = float(hours_amount)

        print('Obliczanie dla 50% kosztów uzyskania przychodu...')
        DocumentRenderer.init().save_processed(self.get_50_data(hours_amount), '50%')
        print('Obliczanie dla 20% kosztów uzyskania przychodu...')
        DocumentRenderer.init().save_processed(self.get_20_data(hours_amount), '20%')


# Document rendering class
class DocumentRenderer:
    _content = ''

    # Read template from file
    def __init__(self):
        with codecs.open('data/template.html', 'r', "utf-8") as content_file:
            self._content = content_file.read()

    # Return new instance
    @staticmethod
    def init():
        return DocumentRenderer()

    # Set data inside templates
    def _set_data(self, data):
        for (key, value) in data.items():
            if isinstance(value, float) or isinstance(value, int):
                value = '%.2f' % value
            self._content = self._content.replace('{{ %s }}' % key, value)

    # Save processed template to tmp file and render it into pdf file
    def save_processed(self, calculated, type=''):
        self._set_data(Config.get('invoice-data'))
        self._set_data(calculated)

        # Save tmp file which will be src for wkhtmltopdf
        with codecs.open('tmp.html', 'w', "utf-8") as res_file:
            res_file.write(self._content)
        # Create dir for pdf files
        try:
            os.mkdir(Config.get('invoice-data')['today_date'])
        except:
            pass
        # Call wkhtmltopdf to generate pdf
        # todo coś innego niż call (?)
        call(["wkhtmltopdf", "tmp.html", "%s/%s %s %s.pdf" % (
            Config.get('invoice-data')['today_date'],
            Config.get('invoice-data')['person_name'],
            type,
            Config.get('invoice-data')['today_date']
        )])
        os.remove('tmp.html')


# Converting number (int) to human-readable string representation in polish
class NumberToString:
    def _thousands(self, num):
        num = int(num)
        if num == 1:
            return 'tysiąc'
        return '%s tysiące' % self._digit(num)

    def _hundreds(self, num):
        num = int(num)
        if num == 0:
            return ''
        elif num == 1:
            return 'sto'
        elif num == 2:
            return 'dwieście'
        elif num == 3:
            return 'trzysta'
        elif num == 4:
            return 'czterysta'
        return '%sset' % self._digit(num)

    def _tens(self, num, last_digit=0):
        num = int(num)
        if num == 0:
            return ''
        elif num == 1:
            if last_digit == 0:
                return 'dziesięć'
            elif last_digit == 1:
                return 'jedenaście'
            elif last_digit in [2, 3, 7, 8]:
                return '%snaście' % self._digit(last_digit)
            elif last_digit == 4:
                return 'czternaście'
            elif last_digit == 5:
                return 'piętnaście'
            elif last_digit == 6:
                return 'szesnaście'
            elif last_digit == 9:
                return 'dziewiętnaście'
        elif num == 2:
            return 'dwadzieścia'
        elif num == 3:
            return 'trzydzieści'
        elif num == 4:
            return 'czterdzieści'
        return '%sdziesiąt' % self._digit(num)

    @staticmethod
    def _digit(num):
        num = int(num)
        return {
            1: 'jeden',
            2: 'dwa',
            3: 'trzy',
            4: 'cztery',
            5: 'pięć',
            6: 'sześć',
            7: 'siedem',
            8: 'osiem',
            9: 'dziewięć',
        }.get(int(num), '')

    # Process whole number and return resulting string representation
    def process(self, num):
        num = str(int(num))
        res = ''
        if len(num) == 4:
            res = '%s ' % self._thousands(num[0])
            num = num[1:]
        if len(num) == 3:
            res = '%s%s ' % (res, self._hundreds(num[0]))
            num = num[1:]
        if len(num) == 2 and num[0] == 1:
            res = '%s%s ' % (
                res, self._tens(num[0], int(num[1]))
            )
        elif len(num) == 2:
            res = '%s%s %s' % (res, self._tens(num[0]), self._digit(num[1]))
        return res
