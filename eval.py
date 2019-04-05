
correct = {}
with open('ans_train.csv', 'r') as f:
	for line in f:
		qid, ans = line.split(',')
		correct[qid] = ans.split(' ')

scores = []
with open('ans.csv', 'r') as f:
	ppreci = []
	for line in f:
		qid, ans = line.split(',')
		docs = ans.split(' ')
		match = 0
		preci = []
		for i,d in enumerate(docs):
			if d in correct[qid]:
				match += 1
				preci.append(match / (i+1))
		ppreci.append(sum(preci)/len(preci))
	print(sum(ppreci)/len(ppreci))
