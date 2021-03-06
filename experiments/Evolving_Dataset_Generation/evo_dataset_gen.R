### create an empty DSD_MG
stream <- DSD_MG(dim = 500)
stream
### add  clusters
# generating the centroids for the clusters
mu <- matrix(runif(500*10, min=-0.5, max=0.5), ncol=500)  
write.csv(mu, "centroid500.csv", row.names=FALSE)

# generation of two gaussian shaped static clusters with  s.d=0.5
c1 <- MGC_Static(den = 100, center=c(mu[1,]), par=0.5, shape = MGC_Shape_Gaussian)
add_cluster(stream, c1 ,label=1)
stream

c2 <- MGC_Static(den = 100, center=c(mu[2,]), par=0.5, shape = MGC_Shape_Gaussian)
add_cluster(stream, c2,label=2)
stream

# generation of 4 gaussian shaped clusters 2 increasing in size and 2 decreasing in size.
c3 <- MGC_Function(
  density = function(t){100},
  parameter = function(t){10*t},
  center = function(t) c(mu[3,]),
  shape = MGC_Shape_Gaussian
)
add_cluster(stream,c3,label=3)
stream

c4 <- MGC_Function(
  density = function(t){100},
  parameter = function(t){1*t},
  center = function(t) c(mu[4,]),
  shape = MGC_Shape_Gaussian
)
add_cluster(stream,c4,label=4)
stream

c5 <- MGC_Function(
  density = function(t){1*100},
  parameter = function(t){1000/t},
  center = function(t) c(mu[5,]),
  shape = MGC_Shape_Gaussian
)
add_cluster(stream,c5,label=5)
stream

c6 <- MGC_Function(
  density = function(t){1*100},
  parameter = function(t){100/t},
  center = function(t) c(mu[6,]),
  shape = MGC_Shape_Gaussian
)
add_cluster(stream,c6,label=6)
stream

# generation of two gaussian shaped random moving clusters with  s.d=0.5 .
c7 <- MGC_Random(density=100, center=c(mu[7,]), parameter=0.5, shape = MGC_Shape_Gaussian, randomness = 0.5)
add_cluster(stream, c7,label=7)
stream

c8 <- MGC_Random(density=100, center=c(mu[8,]), parameter=0.5,shape = MGC_Shape_Gaussian, randomness = 0.5)
add_cluster(stream, c8,label=8)
stream

get_clusters(stream)
###animate_data(stream,30000,xlim=c(-50,50),ylim=c(-50,50), horizon=100)
write_stream(stream, file="datatest500.csv", n=30000, block=1000L , class=TRUE ,sep = "," )

remove_cluster(stream,3)
get_clusters(stream)
write_stream(stream, file="datatest500.csv", n=10000,  block=1000L , class=TRUE , append = TRUE, sep = "," )

remove_cluster(stream,3)
get_clusters(stream)
write_stream(stream, file="datatest500.csv", n=10000,  block=1000L , class=TRUE , append = TRUE, sep = "," )

remove_cluster(stream,3)
get_clusters(stream)
write_stream(stream, file="datatest500.csv", n=10000,  block=1000L , class=TRUE , append = TRUE, sep = "," )

remove_cluster(stream,3)
get_clusters(stream)
write_stream(stream, file="datatest500.csv", n=10000,  block=1000L , class=TRUE , append = TRUE, sep = "," )

remove_cluster(stream,3)
get_clusters(stream)
write_stream(stream, file="datatest500.csv", n=10000,  block=1000L , class=TRUE , append = TRUE, sep = "," )

remove_cluster(stream,3)
get_clusters(stream)
write_stream(stream, file="datatest500.csv", n=10000,  block=1000L , class=TRUE , append = TRUE, sep = "," )