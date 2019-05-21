from bs4 import BeautifulSoup
import requests, json
from tqdm import tqdm
from multiprocessing import Pool

url2content_name = 'url2content.json'
num_threads = 8 
urlcontents = json.load(open(url2content_name, 'r'))

def get_title(url):
    try:
        r = requests.get(url)
    except requests.exceptions.MissingSchema:
        return (url, '')
    soup = BeautifulSoup(r.content, features="html.parser")
    if soup.title == None:
        # print('[warning]', url, 'has no title!')
        title = ''
    else:
        title = str(soup.title).replace('<title>', '').replace('</title>', '')
    return (url, title)

p = Pool(num_threads)
chunksize = 1
pairs = list(tqdm(p.imap_unordered(get_title, urlcontents.keys(), chunksize=chunksize), total=len(urlcontents)))


url2titles = {u:t for u, t in pairs}
json.dump(url2titles, open('url2titles.json', 'w'))
print("done")
