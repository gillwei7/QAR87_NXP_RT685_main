import pkg_resources
import platform
import sys

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

#3.6.2
x,y,z = map(str, sys.version_info[:3])
python_v = x + '.' + y + '.' + z
   
print(python_v + "\n")

vermap = {
    "numpy"      : "1.17.5",
    "protobuf"   : "3.11.2",
    "Pillow"     : "5.4.1",
    "onnx"       : "1.6.0",
    "six"        : "1.14.0",
    "tensorflow" : "1.14.0",
    "py-ubjson"  : "0.14.0",
    "tf2onnx"    : "1.7.1",
    "cython"     : "0.29.14",
    "scipy"      : "1.3.1"
    }
    
out_str = ''   
allOkay = True
for e in vermap:
    ev = vermap[e]
    try :
    	gv = pkg_resources.get_distribution(e).version
    	if(ev == gv):
            print("module: " + e + " , expected: " + ev + " , actual: " + gv + "  ------------------OK")
    	else:
    	    print("module: " + e + " , expected: " + ev + " , actual: " + gv)
    	    allOkay = False
    except :
        allOkay = False
        print('Required Module not found: ' + e + " , expected version : " + ev)


if(allOkay) :
    print("\n\n\n-------------------------ALL OKAY------------------------------------")
