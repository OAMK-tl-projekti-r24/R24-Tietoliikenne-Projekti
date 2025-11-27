import requests
import csv

response= requests.get("http://172.20.241.9/luedataa_kannasta_groupid_csv.php?groupid=24")

with open(r"C:\Users\lrzho\Documents\R24-Tietoliikenne-Projekti\data.csv", "w", encoding="utf-8") as f:
    f.write(response.text)





