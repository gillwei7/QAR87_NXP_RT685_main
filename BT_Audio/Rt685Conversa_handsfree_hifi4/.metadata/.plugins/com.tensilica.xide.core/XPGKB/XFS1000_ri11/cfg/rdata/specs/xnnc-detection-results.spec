

what          XNNC COCO Detection Profile Results`1`Accuracy
scale         Accuracy
name_ex       ^Profile is (.*)_[A-Za-z0-9]+.yaml.*$

start_ex      ^Profile is.*

rownames_ex   ^Profile is ([A-Za-z0-9_]+.yaml)\s*$
colnames_s    AP with IoU = 0.5:0.95`AP with IoU = 0.5:1.0`AP with IoU = 0.75:1
values_ex     ^AP\s*with\s*IoU=.*?\s*=\s*([\x2E\d]*),\s*AP\s*with\s*IoU=.*?\s*=\s*([\x2E\d]*),\s*AP\s*with\s*IoU=.*?\s*=\s*([\x2E\d]*).*$