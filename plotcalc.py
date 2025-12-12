import numpy as np
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D

data = np.genfromtxt('accel_k.csv', delimiter=',')
print(data)

# Uncomment below and comment np.genfromtxt to simulate data

#data = np.zeros((600,4))
#data[0:100,:] = np.array([1800,1500,1500,0])
#data[100:200,:] = np.array([1200,1500,1500,1])
#data[200:300,:] = np.array([1500,1800,1500,2])
#data[300:400,:] = np.array([1500,1200,1500,3])
#data[400:500,:] = np.array([1500,1500,1800,4])
#data[500:600,:] = np.array([1500,1500,1200,5])
#data[:,0:3] = data[:,0:3] + 70*np.random.rand(600, 3)


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

np.savetxt("keskipiste.h", cpoint, fmt="%d", newline='\n', encoding=None)

#otetaan järjestyksessä x, y, z minimi ja maksimiarvo jolloin pisteet tulevat haluttuun järjestykseen
'''
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
'''
#tulostetaan keskipisteet.h tiedostoon
#with open("keskipiste.h", 'w') as f:
#  f.write("int CP [6][3] = {\n")
#  for l in range(5):
#    line = ", ".join(map(str, opoints[l]))
#    f.write("{" + line + "},\n")
#  line = ", ".join(map(str, opoints[5]))
#  f.write("{" + line + "}\n")
#  f.write("};")

# Oletetaan että data ja cpoint on jo olemassa
# data: N x 3 (x, y, z)
# cpoint: k x 3 (keskiarvopisteet)

# Esimerkki (poista nämä jos sinulla on oikea data)
# data = np.random.rand(100,3)
# cpoint = np.array([[0.2,0.3,0.5],[0.7,0.8,0.9]])

fig = plt.figure()
ax = fig.add_subplot(111, projection='3d')

# Piirretään data
ax.scatter(data[:,0], data[:,1], data[:,2], c='blue', alpha=0.01, label='data')

# Piirretään keskipisteet
for i in range(6):
  ax.scatter(cpoint[i,0],cpoint[i,1],cpoint[i,2], c='red')

ax.set_xlabel('X')
ax.set_ylabel('Y')
ax.set_zlabel('Z')
ax.legend()
plt.show()
