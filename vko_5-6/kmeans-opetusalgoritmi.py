def kmeans(data, k = 6, iters = 10):
  minv = data.min()
  maxv = data.max()

  #size määrittää tässä numpy arrayn "leveyden", joten koodi toimii dynaamisesti
  #Voidaan siis lisätä vaikka dataan suunta ilman suurempia lisämuokkauksia koodiin
  size = data.shape[1]

  cpoint = np.random.uniform(minv,maxv,(k,size))
  for itcount in range(iters):
    #luodaan taulukko jossa paikat kuudelle x,y,z arvolle ja arvojen määrälle
    cum = np.zeros((k, size+1))
    #for loop käy läpi pisteet ja summaa ne indeksin mukaan oikealle riville
    for point in data:
      distance = np.linalg.norm(cpoint - point,axis = 1)
      #print("etäisyys =", distance)
      index = np.argmin(distance)
      cum[index,0:size] = cum[index,0:size]+point
      cum[index,size] += 1

    #luodaan/alustetaan taulukko uusille keskipisteille (zeros_like luo zeros taulukon aiemman taulukon mukaiseen muotoon)
    new_cpoint = np.zeros_like(cpoint)
    for j in range(k):
      if cum[j, size] > 0:
        new_cpoint[j] = cum[j, 0:size] / cum[j, size]
        #print("jaettava: ",cum[j,0:size])
        #print("jakaja: ",cum[j,size])
      else:
        new_cpoint[j] = np.random.uniform(minv, maxv, size)
    #tarkastetaan onko muutoksia jos ei ole niin keskeytetään
    if np.allclose(cpoint,new_cpoint):
      print("tavoite saavutettu\nSuoritettu ",itcount," kertaa")
      break
    #print(cum)
    #print("vanhat keskipisteet:\n",cpoint)
    cpoint = new_cpoint
    #print("uudet keskipisteet:\n",cpoint)
  return cpoint

cpoint = kmeans(data)

#luodaan lista johon voidaan laittaa haetut arvot
opoints = []

#otetaan järjestyksessä x, y, z minimi ja maksimiarvo jolloin pisteet tulevat haluttuun järjestykseen
max_x = cpoint[np.argmax(cpoint[:,0])]
min_x = cpoint[np.argmin(cpoint[:,0])]
opoints.append(max_x)
opoints.append(min_x)

max_y = cpoint[np.argmax(cpoint[:,1])]
min_y = cpoint[np.argmin(cpoint[:,1])]
opoints.append(max_y)
opoints.append(min_y)

max_z = cpoint[np.argmax(cpoint[:,2])]
min_z = cpoint[np.argmin(cpoint[:,2])]
opoints.append(max_z)
opoints.append(min_z)

#tulostetaan keskipisteet.h tiedostoon
with open("keskipiste.h", 'w') as f:
  f.write("int CP [6][3] = {\n")
  for l in range(5):
    line = ", ".join(map(str, opoints[l]))
    f.write("{" + line + "},\n")
  line = ", ".join(map(str, opoints[5]))
  f.write("{" + line + "}\n")
  f.write("};")