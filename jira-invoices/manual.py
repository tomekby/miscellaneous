from tools import *

if len(sys.argv) < 2:
    sys.exit('Usage: %s amount' % sys.argv[0])

processor = BasicProcessor(Config.get('calculator')['amount-on-50%'], Config.get('calculator')['amount-on-20%'])
processor.generate_pdf(int(sys.argv[1]))