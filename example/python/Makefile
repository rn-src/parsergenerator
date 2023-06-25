all: exampleG.py exampleL.py

exampleG.py: example.G
	parser --py example.G

exampleL.py: example.L
	tokenizer --py example.L

clean:
	-rm exampleG.py > /dev/null 2>&1
	-rm exampleL.py > /dev/null 2>&1
