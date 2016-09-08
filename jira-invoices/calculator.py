from w3lib.html import remove_tags
from requests import session, codes
from bs4 import BeautifulSoup


# Net/gross calculator for student under 26 years
class Student:
    _hours = 0
    _wage = 0
    _tax_rate = 18
    _cost = 20

    def __init__(self, hours, wage, cost):
        self._hours = hours
        self._wage = wage
        self._cost = cost

    def _get_real_tax_rate(self):
        tax_from = (100 - self._cost) / 100
        return tax_from * self._tax_rate / 100

    def get_net(self):
        return self._wage * self._hours

    def get_gross(self):
        value = self.get_net() / (1 - self._get_real_tax_rate())
        return int(value + 0.5)

    def get_tax_base(self):
        return self.get_gross() - self.get_cost()

    def get_cost(self):
        return self.get_gross() - self.get_gross() * (100 - self._cost) / 100

    def get_tax(self):
        return self.get_gross() - self.get_net()

    def get_cost_percentage(self):
        return self._cost
        

# Net/gross calculator using web client with optional fallback
class WebCalculator:
    _data = None
    _calculator = None
    _cost = 0

    def __init__(self, hours, wage, cost):
        from tools import Config

        self._cost = cost
        self._data = Config.get_calculator_bot().parse(hours * wage, 1 if cost == 50 else 0)
        # Check if bot returned some data
        if self._data == None:
            self._calculator = Config.get_fallback_calculator()(hours, wage, cost)
    
    def get_net(self):
        if self._data == None:
            return self._calculator.get_net()
        return self._data['net']

    def get_gross(self):
        if self._data == None:
            return self._calculator.get_gross()
        return self._data['gross']

    def get_tax_base(self):
        if self._data == None:
            return self._calculator.get_tax_base()
        return self._data['tax_base']

    def get_cost(self):
        if self._data == None:
            return self._calculator.get_cost()
        return self._data['cost']

    def get_tax(self):
        if self._data == None:
            return self._calculator.get_tax()
        return self._data['tax']

    def get_cost_percentage(self):
        return self._cost

    
# Bot finding invoice values on wfirma.pl calculator page
class WfirmaPlBot:
    _url = 'https://poradnik.wfirma.pl/staff_contract_headers/evaluate/errand'

    # Send needed data
    @staticmethod
    def parse(net, copyright):
        from tools import Config

        # Prepare data for request
        form_data = Config.get('wfirma.pl')
        header_data = {
            'quota_type': form_data['quota_type'],
            'quota': net,
            'company_incidental': form_data['company_incidental'],
        }
        form_data['copyright'] = copyright

        with session() as c:
            # convert data to format viable for url-encoding
            data = {}
            for k, v in form_data.items():
                data['data[StaffContractErrand][%s]' % k] = v
            for k, v in header_data.items():
                data['data[StaffContractHeader][%s]' % k] = v

            # Send the request to the server
            try:
                request = c.post(WfirmaPlBot._url, data=data, timeout=3)
            except:
                print('Timeout')
                return None

            # There was some error (most likely server-side), so use offline fallback
            if request.status_code != codes.ok:
                print('Web bot failure')
                return None
                
            return WfirmaPlBot._parse_results(request.text)

    # Parse data returned on request
    @staticmethod
    def _parse_results(request_body):
        # extract wanted data
        soup = BeautifulSoup(request_body.replace('\n', ''), 'xml')
        interesting_columns = soup.findAll('td')[1:15:2]

        # convert to floats
        interesting_columns = list(map(lambda x: float(x.get_text().replace(' ', '').replace(',', '.')), interesting_columns))
        column_names = [
            'net', 'gross', 'all_cost', 'insurance_base', 'cost', 'tax_base', 'tax',
        ]
        result = {}
        for i in range(0, 7):
            result[column_names[i]] = interesting_columns[i]
        return result