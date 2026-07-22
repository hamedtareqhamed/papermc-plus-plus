import json
import os
import glob

def test_tags():
    tags_dir = '/home/hamed/projectes/papermc++/mc/generated/data/minecraft/tags'
    categories = os.listdir(tags_dir)
    print(categories)

if __name__ == '__main__':
    test_tags()
