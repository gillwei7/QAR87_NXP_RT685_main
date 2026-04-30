import sys
import platform
import os

onnx_available = 1
PIL_available = 1
six_available = 1
matplotlib_available = 1
numpy_available = 1

try :
    import onnx
except ModuleNotFoundError:
    onnx_available = 0

try :
    import PIL
except ModuleNotFoundError:
    PIL_available = 0

try :
    import six
except ModuleNotFoundError:
    six_available = 0

try :
    import matplotlib
except ModuleNotFoundError:
    matplotlib_available = 0

try :
	import numpy
except ModuleNotFoundError:
    numpy_available = 0


arg_len=len(sys.argv)

PYTHON_PATH = sys.argv[1]
XNNC_PATH = sys.argv[2]
WORKING_DIR =sys.argv[3]
CFG_PATH = sys.argv[4]
COMPILER_ARG = ""

for i in range(5,arg_len):
    COMPILER_ARG = COMPILER_ARG + sys.argv[i] + " "

execute =  "\"" + PYTHON_PATH + "\"" + " " + XNNC_PATH + " " +"-c"+ " "+ CFG_PATH + " " + COMPILER_ARG

print("\n\n\nExecuting XNNC Command :\n\n"+execute+"\n\n\n")
sys.stdout.flush()

if(platform.python_version()!='3.6.1'):
    print("Warning: Module version availability  for  Python3 . Available version is  "+ platform.python_version() +
          " Required is 3.6.1\n")
sys.stdout.flush()
if(numpy_available == 1 and numpy.__version__ != '1.17.5'):
    print("Warning: Module version availability  for  Numpy . Available version is  "+ numpy.__version__ +
          " Required is 1.17.5\n")
elif(numpy_available == 0):
    print("Warning: Module named Numpy not available . Requires module Numpy with version 1.17.5\n")	
sys.stdout.flush()
if(PIL_available == 1 and PIL.__version__ != '5.4.1'):
    print("Warning: Module version availability  for  Pillow . Available version is  "+ PIL.__version__ +
          " Required is 5.4.1\n")
elif(PIL_available == 0):
    print("Warning: Module named Pillow not available . Requires module Pillow with version 5.4.1\n")		
sys.stdout.flush() 
if(matplotlib_available == 1 and matplotlib.__version__ != '3.1.1'):
    print("Warning: Module version availability  for  Matplotlib . Available version is  "+ matplotlib.__version__ +
          " Required is 3.1.1\n")
elif(matplotlib_available == 0):
    print("Warning: Module named Matplotlib not available . Requires module Matplotlib with version 3.1.1\n")		
sys.stdout.flush()
if(onnx_available == 1 and onnx.__version__ != '1.6.0'):
    print("Warning: Module version availability  for  ONNX . Available version is  "+ onnx.__version__ +
          " Required is 1.6.0\n")
elif(onnx_available == 0):
    print("Warning: Module named onnx not available . Requires module onnx with version 1.6.0\n")		
sys.stdout.flush()
if(six_available == 1 and six.__version__ != '1.14.0'):
    print("Warning: Module version availability  for  six . Available version is  "+ six.__version__ +
          " Required is 1.14.0\n")
elif(six_available == 0):
    print("Warning: Module named six not available .  Requires module six with version 1.14.0\n")		
sys.stdout.flush()

if(os.path.exists(PYTHON_PATH)):
    if(os.path.exists(XNNC_PATH)):
        if(os.path.exists(WORKING_DIR)):
            if(os.path.exists(CFG_PATH)):
                os.chdir(WORKING_DIR)
                return_code=os.system(execute)
                if(return_code == 0):
                    print("\nXNNC compilation completed")
                else:
                    print("\nXNNC compilation completed with errors")
            else:
                print("Path doen't exist:" + CFG_PATH)
        else:
            print("Path doens't exist:" + WORKING_DIR)
    else:
        print("Path doesn't exist:"+ XNNC_PATH)
else:
    print("Path doesn't exist:" + PYTHON_PATH)