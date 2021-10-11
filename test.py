import random
import os

totalElements = 0

def isSorted(l):
  for i in range(1, len(l) - 1):
    if(l[i-1] > l[i]):
      return False
  return True

def generateRandomTest(id: int):
  global totalElements

  blockSize = random.randint(1, 50) 
  consumers = random.randint(1, 50)

  inputElements = []
  outputElement = []

  with open("input.txt", "w+") as inputFile:
    totalElements = random.randint(1, 50) * 10 * blockSize
    current = 0
    inputFile.write(str(totalElements)+"\n")
    while current < totalElements:
      dado = random.randint(1, 1e8)
      inputFile.write(str(dado)+"\n")
      inputElements.append(dado)
      current += 1

  print(f"Testando com {blockSize} bloco(s) e {consumers} consumidores e {totalElements} elementos.")

  os.system(f"gcc main.c -pthread && ./a.out {consumers} {blockSize} input.txt output.txt")

  with open("output.txt", "r") as outputFile:
    for line in outputFile.readlines():
      numbers = line.split(" ")
      numbers.pop()
      numbers = [int(i) for i in numbers]
      if not isSorted(numbers): 
        print("Linha não ordenada!")
        exit()
      for i in numbers: 
        outputElement.append(i)    

  inputElements.sort()
  outputElement.sort()

  if(inputElements == outputElement):
    print(f"\033[92mTeste {id} concluído, todas as linhas ordenadas e elementos iguais. \033[00m")

def runCurrentTestManyTimes():
  inputElements = []

  with open("input.txt", "r") as inputFile:
    for line in inputFile.readlines():
      numbers = line.split(" ")
      for i in numbers: 
        inputElements.append(int(i))    

  inputElements.pop(0)
  inputElements.sort()

  print(f"Testando com a mesma entrada e variando o tamanho do bloco e consumidores.")
  print(f"Elementos na entrada: {totalElements}")

  for test in range(1, 101):
    outputElement = []
    os.system(f"gcc main.c -pthread && ./a.out 10 10 input.txt output.txt")
    with open("output.txt", "r") as outputFile:
      for line in outputFile.readlines():
        numbers = line.split(" ")
        numbers.pop()
        numbers = [int(i) for i in numbers]
        if not isSorted(numbers): 
          print("Linha não ordenada!")
          exit()
        for i in numbers: 
          outputElement.append(i)    

    outputElement.sort()

    if(inputElements == outputElement):
      print(f"\033[92mTeste {test} concluído.")
    else:
      print(f"Erro no teste {test}")
      print(f"Tamanho do vetor de entrada {len(inputElements)}")
      print(f"Tamanho do vetor de saída {len(outputElement)}")
      exit()

for i in range(1, 100):
  generateRandomTest(i)
runCurrentTestManyTimes()
