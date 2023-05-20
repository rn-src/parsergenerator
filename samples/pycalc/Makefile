calc: calculatorG.py calculatorL.py

calculatorG.py: calculator.G
	parser --py calculator.G

calculatorL.py: calculator.L
	tokenizer --py calculator.L

clean:
	@rm calculatorG.py
	@rm calculatorL.py
