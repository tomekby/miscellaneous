from scrapy.http import Request, FormRequest
import scrapy
from tools import *


# Bot findnig amount of worked hours
class JiraSpider(scrapy.Spider):
    name = 'JiraSpider'
    start_urls = [Config.get('jira-url')]

    # Login into jira
    def parse(self, response):
        print('Pobieranie danych o ilości przepracowanych godzin...')
    
        return [FormRequest.from_response(response, formdata=Config.get('jira-login'),
            callback=self.after_login, errback=self.login_failure)]
        
    def login_failure(self, failure):
        print("Nie udało się zalogować")

    # If login succeeded go to timesheet page
    def after_login(self, response):
        uri = '%s?%s' % (Config.get('jira-timesheet')['url'], Config.get_uri_params())
        return scrapy.Request(response.urljoin(uri), self.parse_hours)

    # Parse amount of worked hours and call pdf generation
    @staticmethod
    def parse_hours(response):
        print('Generowanie rachunków...')
        hours_amount = response.css('td.nav.hours.padding.color-sum::text').extract()[0]
        hours_amount = '%.0f' % float(hours_amount.replace('\n', ''))

        processor = BasicProcessor(Config.get('calculator')['amount-on-50%'], Config.get('calculator')['amount-on-20%'])
        processor.generate_pdf(hours_amount)
