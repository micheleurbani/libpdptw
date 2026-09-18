

This article was downloaded by: [129.173.72.87] On: 08 October 2014, At: 06:35
Publisher: Institute for Operations Research and the Management Sciences (INFORMS)
INFORMS is located in Maryland, USA


# Transportation Science


Publication details, including instructions for authors and subscription information:
http://pubsonline.informs.org


# An Adaptive Large Neighborhood Search Heuristic for the Pickup and Delivery Problem with Time Windows


Stefan Ropke, David Pisinger,


# To cite this article:


Stefan Ropke, David Pisinger, (2006) An Adaptive Large Neighborhood Search Heuristic for the Pickup and Delivery Problem
with Time Windows. Transportation Science 40(4):455-472. http://dx.doi.org/10.1287/trsc.1050.0135


# Full terms and conditions of use: http://pubsonline.informs.org/page/terms-and-conditions


This article may be used only for the purposes of research, teaching, and/or private study. Commercial use
or systematic downloading (by robots or other automatic processes) is prohibited without explicit Publisher
approval, unless otherwise noted. For more information, contact permissions@informs.org.


The Publisher does not warrant or guarantee the article's accuracy, completeness, merchantability, fitness
for a particular purpose, or non-infringement. Descriptions of, or references to, products or publications, or
inclusion of an advertisement in this article, neither constitutes nor implies a guarantee, endorsement, or
support of claims made of that product, publication, or service.


Copyright © 2006, INFORMS


# Please scroll down for article-it is on subsequent pages


INFORMS is the largest professional society in the world for professionals in the fields of operations research, management
science, and analytics.
For more information on INFORMS, its publications, membership, or meetings visit http://www.informs.org




## TRANSPORTATION SCIENCE


Vol. 40, No. 4, November 2006, pp. 455-472
issn 0041-1655eissn1526-54470640040455


# informs®


doi 10.1287/trsc.1050.0135


© 2006 INFORMS


# An Adaptive Large Neighborhood Search Heuristic for the Pickup and Delivery Problem with Time Windows


# Stefan Ropke, David Pisinger


DIKU, University of Copenhagen, Universitetsparken 1, DK-2100 Copenhagen, Denmark
{sropke@diku.dk, pisinger@diku.dk}


The pickup and delivery problem with time windows is the problem of serving a number of transportation requests
using a limited amount of vehicles. Each request involves moving a number of goods from a pickup location
to a delivery location. Our task is to construct routes that visit all locations such that corresponding pickups
and deliveries are placed on the same route, and such that a pickup is performed before the corresponding
delivery. The routes must also satisfy time window and capacity constraints.
This paper presents a heuristic for the problem based on an extension of the large neighborhood search heuristic
previously suggested for solving the vehicle routing problem with time windows. The proposed heuristic is
composed of a number of competing subheuristics that are used with a frequency corresponding to their historic
performance. This general framework is denoted adaptive large neighborhood search.
The heuristic is tested on more than 350 benchmark instances with up to 500 requests. It is able to improve
the best known solutions from the literature for more than 50% of the problems.
The computational experiments indicate that it is advantageous to use several competing subheuristics instead
of just one. We believe that the proposed heuristic is very robust and is able to adapt to various instance
characteristics.


Key words: pickup and delivery problems with time windows; large neighborhood search; metaheuristics
History: Received: July 2004; revision received: August 2005; accepted: August 2005.


# Introduction


In the considered variant of the pickup and delivery
problem with time windows (PDPTW), we are given
a number of requests and vehicles. A request consists
of picking up goods at one location and delivering
these goods to another location. Two time windows
are assigned to each request: a pickup time window
that specifies when the goods can be picked up and a
delivery time window that tells when the goods can
be dropped off. Furthermore, service times are associ-
ated with each pickup and delivery. The service times
indicate how long it will take for the pickup or deliv-
ery to be performed. A vehicle is allowed to arrive
at a location before the start of the time window of
the location, but the vehicle must then wait until the
start of the time window before initiating the opera-
tion. A vehicle may never arrive at a location after the
end of the time window of the location.
Each request is assigned a set of feasible vehicles.
This can, for example, be used to model situations
where some vehicles cannot enter a certain location
because of the dimensions of the vehicle.
Each vehicle has a limited capacity, and it starts
and ends its duty at given locations called start and


end terminals. The start and end location do not need
to be the same, and two vehicles can have different
start and end terminals. Furthermore, each vehicle is
assigned a start and end time. The start time indicates
when the vehicle must leave its start location, and the
end time denotes the latest allowable arrival at its end
location. Note that the vehicle leaves its depot at the
specified start time even though this may introduce a
waiting time at the first location visited.
Our task is to construct valid routes for the vehi-
cles. A route is valid if time windows and capacity
constraints are obeyed along the route, each pickup
is served before the corresponding delivery, corre-
sponding pickup and deliveries are served on the
same route, and the vehicle serves only requests
it is allowed to serve. The routes should be con-
structed such that they minimize the cost function to
be described below.
As the number of vehicles is limited, we might
encounter situations where some requests cannot be
assigned to a vehicle. These requests are placed in a
virtual request bank. In a real-world situation it is up
to a human operator to decide what to do with such
requests. The operator might, for example, decide to


455




456


rent extra vehicles in order to serve the remaining
requests.
The objective of the problem is to minimize a
weighted sum consisting of the following three com-
ponents: (1) the sum of the distance traveled by the
vehicles, (2) the sum of the time spent by each vehicle.
The time spent by a vehicle is defined as its arrival
time at the end terminal minus its start time (which
is given a priori), (3) the number of requests in the
request bank. The three terms are weighted by the
coefficients , , and , respectively. Normally, a high
value is assigned to  to serve as many requests as
possible. A mathematical model is presented in §1 to
define the problem precisely.
The problem was inspired from a real-life vehi-
cle routing problem related to transportation of raw
materials and goods between production facilities of
a major Danish food manufacturer. For confidentiality
reasons, we are not able to present any data about the
real-life problem that motivated this research.
The problem is NP-hard, as it contains the traveling
salesman problem as a special case. The objective of
this paper is to develop a method for finding good,
but not necessarily optimal, solutions to the prob-
lem described above. The developed method should
preferably be reasonably fast, robust, and able to han-
dle large problems. Thus, it seems fair to turn to
heuristic methods.
The next paragraphs survey recent work on the
PDPTW. Although none of the references mentioned
below consider exactly the same problem as ours,
they all face the same core problem.
Nanry and Barnes (2000) are among the first
to present a metaheuristic for the PDPTW. Their
approach is based on a reactive tabu search algo-
rithm that combines several standard neighborhoods.
To test the heuristic, Nanry and Barnes create PDPTW
instances from a set of standard vehicle routing prob-
lem with time windows (VRPTW) problems pro-
posed by Solomon (1987). The heuristic is tested
on instances with up to 50 requests. Li and Lim
(2001) use a hybrid metaheuristic to solve the prob-
lem. The heuristic combines simulated annealing and
tabu search. Their method is tested on the nine
largest instances from Nanry and Barnes (2000), and
they consider 56 new instances based on Solomon's
VRPTW problems (1987). Lim, Lim, and Rodrigues
(2002) apply "squeaky wheel" optimization and local
search to the PDPTW. Their heuristic is tested on the
set of problems proposed by Li and Lim (2001). Lau
and Liang (2001) also apply tabu search to PDPTW,
and they describe several construction heuristics for
the problem. Special attention is given to how test
problems can be constructed from VRPTW instances.
Recently, Bent and Van Hentenryck (2003a) pro-
posed a heuristic for the PDPTW based on large


Ropke and Pisinger: An Adaptive Large Neighborhood Search Heuristic
Transportation Science 40(4), pp. 455-472, © 2006 INFORMS


neighborhood search. The heuristic was tested on the
problems proposed by Li and Lim (2001). The heuris-
tic by Bent and Van Hentenryck is probably the most
promising metaheuristic for the PDPTW proposed so
far.
Gendreau et al. (1998) consider a dynamic ver-
sion of the problem. An ejection chain neighborhood
is proposed, and steepest descent and tabu search
heuristics based on the ejection chain neighborhood
are tested. The tabu search is parallelized, and the
sequential and parallelized versions are compared.
Several column generation methods for PDPTW
have been proposed. These methods include both
exact and heuristic methods. Dumas et al. (1991) were
the first to use column generation for solving PDPTW.
They propose a branch-and-bound method that is
able to handle problems with up to 55 requests.
Xu et al. (2003) consider a PDPTW with several
extra real-life constraints, including multiple time
windows, compatibility constraints, and maximum
driving-time restrictions. The problem is solved using
a column generation heuristic. The paper considers
problem instances with up to 500 requests.
Sigurd, Pisinger, and Sig (2004) solve a PDPTW
problem related to transportation of livestock. This
introduces some extra constraints, such as precedence
relations among the requests, meaning that some
requests must be served before others to avoid the
spread of diseases. The problem is solved to opti-
mality using column generation. The largest problems
solved contain more than 200 requests. A recent sur-
vey of pickup and delivery problem literature was
made by Desaulniers et al. (2002).
The work presented in this paper is based on the
master's thesis of Ropke (2002). In the papers by
Pisinger and Ropke (2005) and Ropke and Pisinger
(2006), it is shown how the heuristic presented in this
paper can be extended to solve a variety of vehicle
routing problems-for example the VRPTW, the mul-
tidepot vehicle routing problem, and the vehicle routing
problem with backhauls.
The rest of this paper is organized as follows:
§1 defines the PDPTW problem formally; §2 describes
the basic solution method in a general context; §3
describes how the solution method has been applied
to PDPTW, and extensions to the method are pre-
sented; §4 contains the results of the computational
tests. The computational test is focused on comparing
the heuristic to existing metaheuristics and evaluating
if the refinements presented in §3 improve the heuris-
tic; §5 concludes the paper.


# 1. Mathematical Model


This section presents a mathematical model of the
problem; it is based on the model proposed by




Ropke and Pisinger: An Adaptive Large Neighborhood Search Heuristic
Transportation Science 40(4), pp. 455-472, © 2006 INFORMS


Desaulniers et al. (2002). The mathematical model
serves as a formal description of the problem. As we
solve the problem heuristically, we do not attempt to
write the model on integer-linear form.
A problem instance of the pickup and delivery
problem contains n requests and m vehicles. The
problem is defined on a graph, P =1 n
 is the
set of pickup nodes, D = n +1 2n
 is the set of
delivery nodes. Request i is represented by nodes i
and i + n. K is the set of all vehicles, K=m. One
vehicle might not be able to serve all requests; as an
example, a request might require that the vehicle has
a freezing compartment. Ki is the set of vehicles that
are able to serve request i, and Pk ⊆ P and Dk ⊆ D
are the set of pickups and deliveries, respectively, that
can be served by vehicle k; thus, for all i and k k ∈
Ki ⇔i∈Pk∧i∈Dk. Requests where Ki = K are called
special requests. Define N =P∪DandNk=Pk∪Dk.Let
k = 2n +k, k ∈ K, and let  
k = 2n +m+k, k ∈ K be
the nodes that represent the start and end terminal,
respectively, of vehicle k. The graph G=VA con-
sists of the nodes V =N ∪ 1 m
 ∪ 
1 
m

and the arcs A=V×V.For each vehicle, we have a
subgraph Gk = VkAk, where Vk =Nk∪k
∪
k
 and
Ak =Vk×Vk. For each edge ij ∈A we assign a dis-
tance dij ≥0andatravel time tij ≥ 0. It is assumed that
distances and times are nonnegative; dij ≥ 0, tij ≥ 0
and that the times satisfy the triangle inequality; tij ≤
til +tlj for all i jl∈V. For the sake of modeling, we
also assume that tin+i +si > 0; this makes elimination
of subtours and the pickup-before-delivery constraint
easy to model.
Each node i ∈ V has a service time si and a time
window aibi . The service time represents the time
needed for loading and unloading, and the time win-
dow indicates when the visit at the particular loca-
tion must start; a visit to node i can only take place
between time ai and bi. A vehicle is allowed to arrive
at a location before the start of the time window, but
it has to wait until the start of the time window before
the visit can be performed. For each node i ∈N, li is
the amount of goods that must be loaded onto the
vehicle at the particular node, li ≥ 0 for i ∈ P, and
li =-li-n for i ∈ D. The capacity of vehicle k ∈ K is
denoted Ck.
Four types of decision variables are used in the
mathematical model. xijk , i j ∈ V, k ∈ K is a binary
variable that is one if the edge between node i and
node j is used by vehicle k and zero otherwise. Sik,
i ∈ V, k ∈ K is a nonnegative integer that indicates
when vehicle k starts the service at location i, Lik,
i ∈V, k ∈K is a nonnegative integer that is an upper
bound on the amount of goods on vehicle k after ser-
vicing node i. Sik and Lik are only well defined when
vehicle k actually visits node i. Finally zi, i ∈ P is a
binary variable that indicates if request i is placed in


457


the request bank. The variable is one if the request is
placed in the request bank and zero otherwise.
A mathematical model is


zi (1)


j ∈Adijxijk +k ∈K


S
kk -ak+i


∈P


min
∈ kK i
Subject to:


- ∈ kKi ∈ jNk xijk +zi =1 ∀i ∈P (2)

- ∈ jVk xijk j - ∈Vk xjn+ik =0 ∀k ∈ K ∀i ∈Pk (3)


j P ∈ k∪
k
 xkjk =1 ∀k ∈K (4)
i D ∈ k∪k
xi
kk =1 ∀k ∈K (5)
i xijk
i -xjik=0 ∀k ∈ K ∀j ∈Nk (6)
∈Vk ∈Vk
xijk =1⇒Sik+si+tij ≤Sjk ∀k ∈ K ∀ ij ∈Ak (7)
ai ≤Sik ≤bi ∀k ∈ K ∀i ∈Vk (8)
Sik ≤Sn+ik ∀k ∈ K ∀i ∈Pk (9)
xijk =1⇒Lik+lj≤Ljk ∀k ∈ K ∀ ij ∈Ak (10)
Lik ≤Ck ∀k ∈ K ∀i ∈Vk (11)
Lkk =L
kk =0 ∀k ∈K (12)
xijk ∈ 01
 ∀k ∈ K ∀ ij ∈Ak (13)
zi ∈ 01
 ∀i ∈P (14)
Sik ≥0 ∀k ∈ K ∀i ∈Vk (15)
Lik ≥0 ∀k ∈ K ∀i ∈Vk (16)
The objective function minimizes the weighted sum
of the distance traveled, the sum of the time spent
by each vehicle, and the number of requests not
scheduled.
Constraint (2) ensures that each pickup location is
visited or that the corresponding request is placed in
the request bank. Constraint (3) ensures that the deliv-
ery location is visited if the pickup location is visited
and that the visit is performed by the same vehicle.
Constraints (4) and (5) ensure that a vehicle leaves
every start terminal and a vehicle enters every end
terminal. Together with constraint (6), this ensures
that consecutive paths between k and  
k are formed
for each k ∈K.
Constraints (7) and (8) ensure that Sik is set cor-
rectly along the paths and that the time windows
are obeyed. These constraints also make subtours
impossible. Constraint (9) ensures that each pickup
occurs before the corresponding delivery. Constraints
(10), (11), and (12) ensure that the load variable is set
correctly along the paths and that the capacity con-
straints of the vehicles are respected.




458


# 2. Solution Method


Local search heuristics are often built on neighbor-
hood moves that make small changes to the current
solution, such as moving a request from one route to
another or exchanging two requests, as in Nanry and
Barnes (2000) and Li and Lim (2001). These kinds of
local search heuristics are able to investigate a huge
number of solutions in a short time, but a solution
is changed only very little in each iteration. It is our
belief that such heuristics can have difficulties in mov-
ing from one promising area of the solution space
to another when faced with tightly constrained prob-
lems, even when embedded in metaheuristics.
One way of tackling this problem is by allowing the
search to visit infeasible solutions by relaxing some
constraints; see, e.g., Cordeau, Laporte, and Mercier
(2001). We take another approach-instead of using
small "standard moves," we use very large moves
that potentially can rearrange up to 30%-40% of all
requests in a single iteration. The price of doing this
is that the computation time needed for performing
and evaluating the moves becomes much larger com-
pared to the smaller moves. The number of solutions
evaluated by the proposed heuristic per time unit is
only a fraction of the solutions that could be evalu-
ated by a standard heuristic. Nevertheless, very good
performance is observed in the computational tests,
as demonstrated in §4.
The proposed heuristic is based on large neighbor-
hood search LNS, introduced by Shaw (1997). The
LNS heuristic has been applied to the VRPTW with
good results (Shaw 1997, 1998; Bent and Van Henten-
ryck 2004). Recently the heuristic has been applied to
the PDPTW as well (Bent and Van Hentenryck 2003a).
The LNS heuristic itself is similar to the ruin and recre-
ate heuristic proposed by Schrimpf et al. (2000).
The pseudocode for a minimizing LNS heuristic is
shown in Algorithm 1. The pseudocode assumes that


<table>
 <tr>
  <th></th>
  <th>1: LNS Heuristic.</th>
 </tr>
 <tr>
  <td>1<br/>2</td>
  <td>LNSs ∈ solutions<br/>q ∈<br/>solution sbest =s;</td>
 </tr>
 <tr>
  <td>3</td>
  <td>repeat</td>
 </tr>
 <tr>
  <td>4</td>
  <td>s<br/> =s;</td>
 </tr>
 <tr>
  <td>5</td>
  <td>remove q requests from s<br/></td>
 </tr>
 <tr>
  <td>6</td>
  <td>reinsert removed requests into s<br/>;</td>
 </tr>
 <tr>
  <td>7</td>
  <td>if (fs<br/>&lt;fsbest) then</td>
 </tr>
 <tr>
  <td>8</td>
  <td>sbest =s<br/>;</td>
 </tr>
 <tr>
  <td>9</td>
  <td>if accepts<br/> s then</td>
 </tr>
 <tr>
  <td>10</td>
  <td>s=s<br/>;</td>
 </tr>
 <tr>
  <td>11<br/>12</td>
  <td>until stop-criterion met<br/>sbest;</td>
 </tr>
</table>


Ropke and Pisinger: An Adaptive Large Neighborhood Search Heuristic
Transportation Science 40(4), pp. 455-472, © 2006 INFORMS


an initial solution s already has been found, for exam-
ple, by a simple construction heuristic. The second
parameter q determines the scope of the search.
Lines 5 and 6 in the algorithm are the interesting
part of the heuristic. In Line 5, a number of requests
are removed from the current solution s
, and in Line 6
the requests are reinserted into the current solution
again. The performance and robustness of the overall
heuristic is very dependent on the choice of removal
and insertion procedures. In the previously proposed
LNS heuristics for VRPTW or PDPTW (Shaw 1997;
Bent and Van Hentenryck 2003a), near-optimal meth-
ods were used for the reinsert operation. This was
achieved using a truncated branch-and-bound search.
In this paper we take a different approach by using
simple insertion heuristics for performing the inser-
tions. Even though the insertion heuristics themselves
usually deliver solutions of poor quality, the quality
of the LNS heuristic is very good, because the bad
moves that are generated by the insertion heuristics
lead to a fruitful diversification of the search process.
The rest of the code updates the (so far) best solu-
tion and determines if the new solution should be
accepted. A simple accept criterion would be to accept
all improving solutions. Such a criterion has been
used in earlier LNS implementations (Shaw 1997).
In this paper we use a simulated annealing accept
criterion.
In Line 11 we check if a stop criterion is met. In
our implementation we stop when a certain number
of iterations has been performed.
The parameter q ∈ 0 n
determines the size of
the neighborhood. If q is equal to zero, then no search
at all will take place because no requests are removed.
On the other hand, if q is equal to n, then the problem
is resolved from scratch in each iteration. In general,
one can say that the larger q is, the easier it is to move
around in the solution space, but when q gets larger,
each application of the insertion procedure is going
to be slower. Furthermore, if one uses a heuristic for
inserting requests, then choosing q too large might
give bad results.
The LNS local search can be seen as an example of
a very large-scale neighborhood search as presented
by Ahuja et al. (2002). Ahuja et al. define very large-
scale neighborhoods as neighborhoods whose sizes
grow exponentially as a function of the problem size,
or neighborhoods that simply are too large to be
searched explicitly in practice. The LNS local search
fits into the last category, as we have a large number
of possibilities for choosing the requests to remove
and a large number of possible insertions. One impor-
tant difference between the proposed heuristic and
most of the heuristics described in Ahuja et al. (2002)
is that the latter heuristics typically examine a huge
number of solutions, albeit implicitly, while the LNS




Ropke and Pisinger: An Adaptive Large Neighborhood Search Heuristic
Transportation Science 40(4), pp. 455-472, © 2006 INFORMS


heuristic proposed in this paper examines only a rel-
atively low number of solutions.
Instead of viewing the LNS process as a sequence
of remove-insert operations, it can also be viewed as
a sequence of fix-optimize operations. In the fix opera-
tion a number of elements in the current solution are
fixed. If, for example, the solution is represented as a
vector of variables, the fix operation could fix a num-
ber of these variables at their current value. The opti-
mize operation then reoptimizes the solution while
respecting the fixation performed in the previous fix
operation. This way of viewing the heuristic might
help us to apply the heuristic to problems where
the remove-insert operations do not seem intuitive.
In §3 we introduce the term adaptive large neighbor-
hood search (ALNS) to describe an algorithm using sev-
eral large neighborhoods in an adaptive way. A more
general presentation of the ALNS framework can be
found in a subsequent paper (Pisinger and Ropke
2005).


# 3. LNS Applied to PDPTW


This section describes how the LNS heuristic has been
applied to the PDPTW. Compared to the LNS heuris-
tic developed for the VRPTW and PDPTW by Shaw
(1997, 1998) and Bent and Van Hentenryck (2003a,
2004), the heuristic in this paper is different in several
ways:


1. We are using several removal and insertion


heuristics during the same search, while the earlier
LNS heuristics used only one method for removal
and one method for insertions. The removal heuris-
tics are described in §3.1 and the insertion heuristics
are described in §3.2. The method for selecting which
subheuristic to use is described in §3.3. The selection
mechanism is guided by statistics gathered during the
search, as described in §3.4. We are going to use the
term adaptive large neighborhood search ALNS heuris-
tic for an LNS heuristic that uses several competing
removal and insertion heuristics and chooses between
using statistics gathered during the search.


2. Simple and fast heuristics are used for the inser-


tion of requests, as opposed to the more complicated
branch-and-bound methods proposed by Shaw (1997,
1998) and Bent and Van Hentenryck (2003a, 2004).


3. The search is embedded in a simulated annealing


metaheuristic where the earlier LNS heuristics used a
simple descent approach. This is described in §3.5.
The present section also describes how the LNS
heuristic can be used in a simple algorithm designed
for minimizing the number of vehicles used to serve
all requests. The vehicle minimization algorithm only
works for homogeneous fleets without an upper
bound on the number of vehicles available.


459


# 3.1. Request Removal


This section describes three removal heuristics. All
three heuristics take a solution and an integer q as
input. The output of the heuristic is a solution where q
requests have been removed. Furthermore, the heuris-
tics Shaw removal and worst removal have a parameter p
that determines the degree of randomization in the
heuristic.


3.1.1. Shaw Removal Heuristic. This removal


heuristic was proposed by Shaw (1997, 1998). In this
section it is slightly modified to suit the PDPTW. The
general idea is to remove requests that are some-
what similar, as we expect it to be reasonably easy
to shuffle similar requests around and thereby cre-
ate new, perhaps better, solutions. If we choose to
remove requests that are very different from each
other, then we might not gain anything when rein-
serting the requests because we might only be able
to insert the requests at their original positions or in
some bad positions. We define the similarity of two
requests i and j using a relatedness measure Rij. The
lower Rij is, the more related are the two requests.
The relatedness measure used in this paper consists
of four terms: a distance term, a time term, a capacity
term, and a term that considers the vehicles that can
be used to serve the two requests. These terms are
weighted using the weights +, ,, -, and ., respec-
tively. The relatedness measure is given by


Rij = +dAiAj +dBiBj
+,TAi -TAj+TBi-TBj
-lj+.1- Ki ∩Kj
minKiKj



+-li
Ai and Bi denote the pickup and delivery loca-
tions of request i, and T indicates the time when
i
location i is visited. dij , li, and Ki are defined in §1.
Using the decision variable S from §1, we can write
ik
T as T
i i =k∈Kj∈Vk Sikxijk. The term weighted by +
measures distance, the term weighted by , measures
temporal connectedness, the term weighted by -
compares capacity demand of the requests, and the
term weighted by . ensures that two requests get a
high relatedness measure if only a few or no vehicles
are able to serve both requests. It is assumed that d ,
ij
T , and l are normalized such that 0
x i ≤ Rij≤2++
,+-+.. This is done by scaling dij , Tx, and li such
that they only take on values from 0 1 . Notice that
we cannot calculate Rij, if request i or j is placed
in the request bank.
The relatedness is used to remove requests in the
same way as described by Shaw (1997). The proce-
dure for removing requests is shown in pseudocode
in Algorithm 2. The procedure initially chooses a
random request to remove, and in the subsequent


-  (17)




460


iterations it chooses requests that are similar to the
already removed requests. A determinism parameter
p ≥1 introduces some randomness in the selection of
the requests (a low value of p corresponds to much
randomness).


Algorithm 2: Shaw Removal.


1 Function ShawRemovals ∈ solutions
,
q ∈p∈+
2 request: r =a randomly selected request
from s;
3 set of requests: D=r
;
4 while D <qdo
5 r =a randomly selected request from D;
6 Array: L=an array containing all request
from s not in D;
7 sort L such that
i<j⇒RrLi <RrLj ;
8 choose a random number y from the
interval 0 1;
9 D=D∪LypL 
;
10 end while
11 remove the requests in D from s;


Notice that the sorting in Line 7 can be avoided in
an actual implementation of the algorithm because it
is sufficient to use a linear time selection algorithm
(Cormen et al. 2001) in Line 9.


3.1.2. Random Removal. The random removal


algorithm simply selects q requests at random and
removes them from the solution. The random removal
heuristic can be seen as a special case of the Shaw
removal heuristic with p=1. We have implemented a
separate random removal heuristic however, because
it obviously can be implemented to run faster than
the Shaw removal heuristic.


3.1.3. Worst Removal. Given a request i served by


some vehicle in a solution s, we define the cost of the
request as costi s = fs-f-is where f-is is the
cost of the solution without request i (the request is
not moved to the request bank, but removed com-
pletely). It seems reasonable to try to remove requests
with high cost and insert them at another place in the
solution to obtain a better solution value; therefore,
we propose a removal heuristic that removes requests
with high costi s.
The worst removal heuristic is shown in pseu-
docode in Algorithm 3. It reuses some of the ideas
from §3.1.1.
Notice that the removal is randomized, with the
degree of randomization controlled by the parame-
ter p as in §3.1.1. This is done to avoid situations
where the same requests are removed over and over
again.


Ropke and Pisinger: An Adaptive Large Neighborhood Search Heuristic
Transportation Science 40(4), pp. 455-472, © 2006 INFORMS


# Algorithm 3: Worst Removal.


1 Function WorstRemovals ∈ solutions
,
q ∈p∈+
2 while q>0do
3 Array: L=All planned requests i, sorted
by descending costi s;
4 choose a random number y in the
interval 0 1;
5 request: r = LypL ;
6 remove r from solution s;
7 q =q-1;
8 end while


One can say that the Shaw removal heuristic and
the worst removal heuristic belong to two different
classes of removal heuristics. The Shaw heuristic is
biased toward selecting requests that can "easily"
be exchanged, while the worst-removal selects the
requests that appear to be placed in the wrong posi-
tion in the solution.


# 3.2. Inserting Requests


Insertion heuristics for vehicle routing problems are
typically divided into two categories: sequential and
parallel insertion heuristics. The difference between
the two classes is that sequential heuristics build one
route at a time, while parallel heuristics construct sev-
eral routes at the same time. Parallel and sequen-
tial insertion heuristics are discussed in further detail
in Potvin and Rousseau (1993). The heuristics pre-
sented in this paper are all parallel. The reader should
observe that the insertion heuristic proposed here will
be used in a setting where they are given a number
of partial routes and a number of requests to insert-
they seldom build the solution from scratch.


3.2.1. Basic Greedy Heuristic. The basic greedy


heuristic is a simple construction heuristic. It per-
forms at most n iterations as it inserts one request in
each iteration. Let 3f denote the change in objec-
i k
tive value incurred by inserting request i into route k
at the position that increases the objective value the
least. If we cannot insert request i in route k, then we
set 3f
i k=. We then define ci as ci = mink∈K3fik
.
In other words, c is the "cost" of inserting request i
i
at its best position overall. We denote this position
by the minimum cost position. Finally, we choose the
request i that minimizes


minci (18)
i∈U


and insert it at its minimum cost position. U is the set
of unplanned requests. This process continues until
all requests have been inserted or no more requests
can be inserted.




Ropke and Pisinger: An Adaptive Large Neighborhood Search Heuristic
Transportation Science 40(4), pp. 455-472, © 2006 INFORMS


461


of k discover earlier that the possibilities for inserting
a request become limited.
Regret heuristics have been used by Potvin and
Rousseau (1993) for the VRPTW. The heuristic in
their paper can be categorized as a regret-k heuris-
tic with k = m, because all routes are considered in
an expression similar to (19). The authors do not
use the change in the objective value for evaluating
the cost of an insertion, but use a special cost func-
tion. Regret heuristics can also be used for combinato-
rial optimization problems outside the vehicle routing
domain, an example of an application to the general-
ized assignment problem described by Martello and
Toth (1981). As in the previous section, we use the
fact that we only change one route in each iteration
to speed up the regret heuristic.


Observe that in each iteration we only change one
route (the one we inserted into), and we do not have
to recalculate insertion costs in all the other routes.
This property is used in the concrete implementation
to speed up the insertion heuristics.
An obvious problem with this heuristic is that it
often postpones the placement of "hard" requests
(requests that are expensive to insert, that is, requests
with large ci) to the last iterations where we do not
have many opportunities for inserting the requests
because many of the routes are "full." The heuristic
presented in the next section tries to circumvent this
problem.


3.2.2. Regret Heuristics. The regret heuristic tries


to improve on the basic greedy heuristic by incorpo-
rating a kind of look-ahead information when select-
ing the request to insert. Let xik ∈ 1 m
 be a
variable that indicates the route for which request i
has the kth lowest insertion cost, that is, 3fixik ≤
3fixik
 for k ≤k
. Using this notation, we can express ci
from §3.2.1 as ci = 3fixi1. In the regret heuristic we
as c∗
define a regret value c∗ i i = 3fixi2 - 3fixi1.
other words, the regret value is the difference in the
cost of inserting the request in its best route and its
second-best route. In each iteration the regret heuris-
tic chooses to insert the request i that maximizes


# 3.3. Choosing a Removal and


an Insertion Heuristic
In §3.1 we defined three removal
random, and worst removal), and in
a class of insertion heuristics (basic insertion,
In


heuristics (Shaw,
§3.2 we defined
regret-
2, regret-3, etc.). One could select one removal and
one insertion heuristic and use these throughout the
search, but in this paper we propose to use all heuris-
tics. The reason for doing this is that, for example,
the regret-2 heuristic may be well suited for one type
of instance, while the regret-4 heuristic may be the
heuristic best suited for another type of instance. We
believe that alternating between the different removal
and insertion heuristics gives us a more robust heuris-
tic overall.
To select the heuristic to use, we assign weights to
the different heuristics and use a roulette wheel selection
principle. If we have k heuristics with weights wi, i ∈
12 k
, we select heuristic j with probability


maxc∗ i
i∈U


The request is inserted at its minimum cost position.
Ties are broken by selecting the insertion with low-
est cost. Informally speaking, we choose the insertion
that we will regret most if it is not done now.
The heuristic can be extended in a natural way to
define a class of regret heuristics: The regret-k heuristic
is the construction heuristic that in each construction
step chooses to insert the request i that maximizes:


wj
 ki=1 wi
Notice that the insertion heuristic
dently of the removal heuristic (and
possible to set these weights by hand,
a quite involved process if many removal
tion heuristics are used. Instead, an adaptive
adjusting algorithm is proposed in §3.4.


(20)


max k
i∈U j= 13fixij -3fixi1


is selected indepen-
vice versa). It is
but it can be
and inser-
weight-


(19)


If some requests cannot be inserted in at least
m-k+1routes, then the request that can be inserted
in the fewest number of routes (but still can be
inserted in at least one route) is inserted. Ties are
broken by selecting the request with best insertion
cost. The request is inserted at its minimum cost posi-
tion. The regret heuristic presented at the start of this
section is a regret-2 heuristic and the basic insertion
heuristic from §3.2.1 is a regret-1 heuristic because of
the tie-breaking rules. Informally speaking, heuristics
with k>2 investigate the cost of inserting a request
on the k best routes and insert the request whose
cost difference between inserting it into the best route
and the k - 1 best routes is largest. Compared to a
regret-2 heuristic, regret heuristics with large values


# 3.4. Adaptive Weight Adjustment


This section describes how the weights wj introduced
in §3.3 can be automatically adjusted using statistics
from earlier iterations. The basic idea is to keep track
of a score for each heuristic, which measures how well
the heuristic has performed recently. A high score cor-
responds to a successful heuristic. The entire search
is divided into a number of segments. A segment is a
number of iterations of the ALNS heuristic; here we
define a segment as 100 iterations. The score of all




462


Table 1


Score Adjustment Parameters


Parameter


Description


1


2


The last remove-insert operation resulted in a
new global best solution.
The last remove-insert operation resulted in a solution
that has not been accepted before. The cost of the
new solution is better than the cost of current solution.
The last remove-insert operation resulted in a solution
that has not been accepted before. The cost of the
new solution is worse than the cost of current solution,
but the solution was accepted.


3


heuristics is set to zero at the start of each segment.
The score of a heuristic is increased by either 71, 72,
or 73 in the situations shown in Table 1.
The case for 71 is clear: If a heuristic is able to find a
new overall best solution, then it has done well. Simi-
larly, if a heuristic has been able to find a solution that
has not been visited before and it is accepted by the
accept criteria in the ALNS search, then the heuris-
tic has been successful, as it has brought the search
forward. It seems sensible to distinguish between
the two situations corresponding to parameters 72
and 73 because we prefer heuristics that can improve
the solution, but we are also interested in heuristics
that can diversify the search, and these are rewarded
by 73. It is important to note that we only reward
unvisited solutions. This is to encourage heuristics
that are able to explore new parts of the solution
space. We keep track of visited solutions by assigning
a hash key to each solution and storing the key in a
hash table.
In each iteration we apply two heuristics: a removal
heuristic and an insertion heuristic. The scores for
both heuristics are updated by the same amount, as
we cannot tell whether it was the removal or the
insertion that was the reason for the "success."
At the end of each segment we calculate new
weights using the recorded scores. Let wij be the
weight of heuristic i used in segment j as the weight
used in formula (20). In the first segment we weight
all heuristics equally. After we have finished seg-
ment j, we calculate the weight for all heuristics i to
be used in segment j+1 as follows:
8
wij+1 =wij1-r+r i
9i
8i is the score of heuristic i obtained during the last
segment, and 9i is the number of times we have
attempted to use heuristic i during the last segment.
The reaction factor r controls how quickly the weight-
adjustment algorithm reacts to changes in the effec-
tiveness of the heuristics. If r is zero, then we do not
use the scores at all and stick to the initial weights.
If r is set to one, then we let the score obtained in the
last segment decide the weight.


Ropke and Pisinger: An Adaptive Large Neighborhood Search Heuristic
Transportation Science 40(4), pp. 455-472, © 2006 INFORMS


7


6


5


4


3


2


1


Worst removal
Random removal
Shaw removal


0
0


5,000 10,000 15,000 20,000 25,000
Iteration


Figure 1 The Figure Shows an Example of How the Weights for the
Three Removal Heuristics Progressed During One Applica-
tion of the Heuristic
Notes. The iteration number is shown along the x-axis and the weight is
shown along the y-axis. The graph illustrates that for the particular prob-
lem, the random removal and the Shaw removal heuristics perform virtually
equally well, while the worst heuristic performs worst. Consequently, the
worst heuristic is not used as often as the two other heuristics.


Figure 1 shows an example of how the weights of
the three removal heuristics progress over time for
a certain problem instance. The plots are decreasing
because of the simulated annealing acceptance criteria
to be described in the next section. Toward the end of
the search we only accept good moves, and therefore
it is harder for the heuristic to get high scores.


# 3.5. Acceptance and Stopping Criteria


As described in §2, a simple acceptance criterion
would be to accept only solutions that are better than
the current solution. This would give us a descent
heuristic like the one proposed by Shaw (1997). How-
ever, such a heuristic has a tendency to get trapped
in a local minimum, so it seems sensible to on occa-
sion accept solutions that are worse than the current
solution. To do this, we use the acceptance criteria
from simulated annealing. That is, we accept a solu-
tion s
 given the current solution s with probability
e-fs
-fs/T where T>0isthetemperature.
The temperature starts out at Tstart and is decreased
every iteration using the expression T


= T · c, where


0<c<1isthecooling rate. A good choice of Tstart
is dependent on the problem instance at hand, so
instead of specifying Tstart as a parameter we calcu-
late Tstart by inspecting our initial solution. First we
calculate the cost z
 of this solution using a modified
objective function. In the modified objective function,


 (cost of having requests in the request bank) is set


to zero. The start temperature is now set such that a
solution that is w% worse than the current solution is
accepted with probability 0.5. The reason for setting 
to zero is that this parameter typically is large and




Ropke and Pisinger: An Adaptive Large Neighborhood Search Heuristic
Transportation Science 40(4), pp. 455-472, © 2006 INFORMS


could cause us to set the starting temperature to a too-
large number if the initial solution had some requests
in the request bank. Now w is a parameter that has to
be set. We denote this parameter the start temperature
control parameter. The algorithm stops when a speci-
fied number of LNS iterations have passed.


# 3.6. Applying Noise to the Objective Function


As the proposed insertion heuristics are quite myopic,
we believe that it is worthwhile to randomize the
insertion heuristics such that they do not always make
the move that seems best locally. This is achieved by
adding a noise term to the objective function. Every
time we calculate the cost C of an insertion of a
request into a route, we also calculate a random num-
ber noise in the interval -maxNmaxN and cal-
culate the modified insertion costs C
 = max0C+
noise
. At each iteration we decide if we should
use C or C
 to determine the insertions to perform.
This decision is taken by the adaptive mechanism
described earlier by keeping track of how often the
noise applied insertions and the "clean" insertions are
successful.
To make the amount of noise related to the prop-
erties of the problem instance, we calculate maxN =
<·maxij∈Vdij
, where < is a parameter that controls
the amount of noise. We have chosen to let maxN be
dependent on the distances dij as the distances are an
important part of the objective in all of the problems
we consider in this paper.
It might seem superfluous to add noise to the
insertion heuristics, as the heuristics are used in a
simulated annealing framework that already contains
randomization; however, we believe that the noise
applications are important as our neighborhood is
searched by means of the insertion heuristics and not
randomly sampled. Without the noise applications we
do not get the full benefit of the simulated anneal-
ing metaheuristic. This conjecture is supported by the
computational experiments reported in Table 4.


# 3.7. Minimizing the Number of Vehicles Used


Minimization of the number of vehicles used to serve
all requests is often considered as first priority in the
vehicle routing literature. The heuristic proposed so
far is not able to cope with such an objective, but by
using a simple two-stage algorithm that minimizes
the number of vehicles in the first stage and then
minimizes a secondary objective (typically traveled
distance) in the second stage, we can handle such
problems. The vehicle minimization algorithm only
works for problems with a homogeneous fleet. We
also assume that the number of vehicles available is
unlimited, such that constructing an initial feasible
solution always can be done. A two-stage method was
also used by Bent and Van Hentenryck (2003a, 2004),


463


but while they used two different neighborhoods and
metaheuristics for the two stages, we use the same
heuristic in both stages.
The vehicle minimization stage works as follows:
First, an initial feasible solution is created using a
sequential insertion method that constructs one route
at a time until all requests have been planned. The
number of vehicles used in this solution is the ini-
tial estimate on the number of vehicles necessary.
The next step is to remove one route from our fea-
sible solution. The requests on the removed route
are placed in the request bank. The resulting prob-
lem is solved by our LNS heuristic. When the heuris-
tic is run, a high value is assigned to , such that
requests are moved out of the request bank if possible.
If the heuristic is able to find a solution that serves
all requests, a new candidate for the minimum num-
ber of vehicles has been found. When such a solu-
tion has been found, the LNS heuristic is immediately
stopped, one more route is removed from the solu-
tion, and the process is reiterated. If the LNS heuris-
tic terminates without finding a solution where all
requests are served, then the algorithm steps back to
the last solution encountered in which all requests
were served. This solution is used as a starting solu-
tion in the second stage of the algorithm, which sim-
ply consists of applying the normal LNS heuristic.
To keep the running time of the vehicle minimiza-
tion stage down, this stage is only allowed to spend


= LNS iterations all together, such that if the first


application of the LNS heuristic, for example, spends
a iterations to find a solution where all requests are
planned, then the vehicle minimization stage is only
allowed to perform =-aLNS iterations to minimize
the number of vehicles further. Another way to keep
the running time limited is to stop the LNS heuris-
tic when it seems unlikely that a solution exists in
which all requests are planned. In practice, this is
implemented by stopping the LNS heuristic if five or
more requests are unplanned and no improvement in
the number of unplanned requests has been found in
the last  LNS iterations. In the computational exper-
iments = was set to 25,000 and  was set to 2,000.


# 3.8. Discussion


Using several removal and insertion heuristics dur-
ing the search may be seen as using local search with
several neighborhoods. To the best of our knowledge,
this idea has not been used in the LNS literature
before. The related Variable Neighborhood Search
(VNS) was proposed by Mladenovi´c and Hansen
(1997). VNS is a metaheuristic framework using a
parameterized family of neighborhoods. The meta-
heuristic has received quite a lot of attention in the
recent years and has provided impressive results for
many problems. Where ALNS makes use of several




464


unrelated neighborhoods, VNS typically is based on
a single neighborhood that is searched with variable
depth.
Several metaheuristics can be used at the top level
of ALNS to help the heuristic escape a local min-
imum. We have chosen to use simulated annealing
because the ALNS heuristic already contains the ran-
dom sampling element. For a further discussion of
metaheuristic frameworks used in connection with
ALNS, see the subsequent paper (Pisinger and Ropke
2005).
The request bank is an entity that makes sense for
many real-life applications. In the problems consid-
ered in §4 we do not accept solutions with unsched-
uled requests, but the request bank allows us to visit
infeasible solutions in a transition stage, improving
the overall search. The request bank is particularly
important when minimizing the number of vehicles.


# 4. Computational Experiments


In this section we describe our computational exper-
iments. We first introduce a set of tuning instances
in §4.1. In §4.2 we evaluate the performance of
the proposed construction heuristics on the tuning
instances. In §4.3 we describe how the parameters
of the ALNS heuristic were tuned, and in §4.4 we
present the results obtained by the ALNS heuristic
and a simpler LNS heuristic.


# 4.1. Tuning Instances


First, a set of representative tuning instances is iden-
tified. The tuning instances must have a fairly limited
size because we want to perform numerous exper-
iments on the tuning problems, and they should
somehow be related to the problems at which our
heuristic is targeted. In the case at hand, we want to
solve some standard benchmark instances and a new
set of randomly generated instances.
Our tuning set consists of 16 instances. The first
four instances are LR1_2_1, LR202, LRC1_2_3, and
LRC204 from Li and Lim's benchmark problems
(2001), containing between 50 and 100 requests. The
number of available vehicles was set to one more than
that reported by Li and Lim to make it easier for
the heuristic to find solutions with no requests in the
request bank. The last 12 instances are randomly gen-
erated instances. These instances contain both single-
depot and multidepot problems and problems with
requests that only can be served by a subset of the
vehicle fleet. All randomly generated problems con-
tain 50 requests.


# 4.2. Evaluation of Construction Heuristics


First, we examine how the simple construction heuris-
tics from §3.2 perform on the tuning problems,


Ropke and Pisinger: An Adaptive Large Neighborhood Search Heuristic
Transportation Science 40(4), pp. 455-472, © 2006 INFORMS


Performance of Construction Heuristics


Table 2


Regret-2 Regret-3 Regret-4 Regret-m


Basic greedy


3 26 0 26  27
3 2 0
002 002 003


Avg. gap (%)
Fails
Time (s)


407
3
002


 30


3
002


Notes. Each column in the table corresponds to one of the construction
heuristics. These simple heuristics were not always able to construct a solu-
tion where all requests are served; hence, for each heuristic we report the
number of times this happened in the fails row. The Avg. gap row shows the
average relative difference between the found solution and the best known
solution. Only solutions where all requests are served are included in the cal-
culations of the average relative difference. The last row shows the average
time (in seconds) needed for applying the heuristic to one problem, running
on a 1.5 GHz Pentium IV.


to see how well they work without the LNS frame-
work. The construction heuristics regret-1, regret-2,
regret-3, regret-4, and regret-m have been imple-
mented. Table 2 shows the results of the test. As the
construction heuristics are deterministic, the results
were produced by applying the heuristics to each of
the 16 test problems once.
The results show that the proposed construction
heuristics are very fast but also very imprecise. Basic
greedy is the worst heuristic, while all the regret
heuristics are comparable with respect to the solution
quality. Regret-m stands out, however, because it is
able to serve all requests in all problems. It would
probably be possible to improve the results shown in
Table 2 by introducing seed requests as proposed by
Solomon (1987). However, we are not going to report
on such experiments in this paper. It might be surpris-
ing that these very imprecise heuristics can be used
as the foundation of a much more precise local search
heuristic, but as we are going to see in the following
sections, this is indeed possible.


# 4.3. Parameter Tuning


This part of the paper serves two purposes. First, it
describes how the parameters used for producing the
results in §4.4 were found. Next, it tries to unveil
which part of the heuristic contributes most to the
solution quality.


4.3.1. Parameters. This section determines the


parameters that need to be tuned. We first review the
removal parameters. Shaw removal is controlled by
five parameters: +, ,, -, ., and p, while the worst
removal is controlled by one parameter pworst. Random
removal has no parameters. The insertion heuristics
are parameter free when we have chosen the regret
degree.
To control the acceptance criteria we use two
parameters, w and c. The weight-adjustment algo-
rithm is controlled by four parameters, 71, 72, 73, and


r. Finally, we have to determine a noise rate < and


a parameter > that control how many requests we




Ropke and Pisinger: An Adaptive Large Neighborhood Search Heuristic
Transportation Science 40(4), pp. 455-472, © 2006 INFORMS


remove in each iteration. In each iteration, we chose a
random number ? that satisfies 4 ≤ ? ≤ min100>n,
and remove ? requests. We stop the search after
25,000 LNS iterations as this resulted in a fair trade-off
between time and quality.


4.3.2. LNS Parameter Tuning. Despite the large


number of parameters used in the LNS heuristic, it
turns out that it is relatively easy to find a set of
parameters that works well for a large range of prob-
lems. We use the following strategy for tuning the
parameters: First, a fair parameter setting is produced
by an ad hoc trial-and-error phase; this parameter set-
ting was found while developing the heuristic. This
parameter setting is improved in the second phase by
allowing one parameter to take a number of values,
while the rest of the parameters are kept fixed. For
each parameter setting we apply the heuristic on our
set of test problems five times, and the setting that
shows the best average behavior (in terms of average
deviation from the best known solutions) is chosen.
We now move on to the next parameter, using the
values found so far and the values from the initial
tuning for the parameters that have not been consid-
ered yet. This process continues until all parameters
have been tuned. Although it would be possible to
process the parameters once again using the new set
of parameters as a starting point to further optimize
the parameters, we stopped after one pass.
One of the experiments performed during the
parameter tuning sought to determine the value of
the parameter >, which controls how many requests
we remove and insert in each iteration. This param-
eter should intuitively have a significant impact on
the results our heuristic is able to produce. We tested
the heuristic with > ranging from 0.05 to 0.5 with a
step size of 0.05. Table 3 shows the influence of >.
When > is too low, the heuristic is not able to move
very far in each iteration, and it has a higher chance
of being trapped in one suboptimal area of the search
space. On the other hand, if > is large, then we can
easily move around in the search space, but we are
stretching the capabilities of our insertion heuristics.
The insertion heuristics work fairly well when they
must insert a limited number of requests into a partial
solution, but they cannot build a good solution from
scratch, as seen in §4.2. The results in Table 3 show
that > = 0 4 is a good choice. One must notice that
the heuristic gets slower when > increases because


Parameter  vs. Solution Quality


Table 3


005 01  015 02  025 03  035 04  045 05
175 165 121 097 081 071 081 049 057 057





Avg. gap (%)


Notes. The first row shows the values of the parameter  that were tested,
and the second row shows the gap between the average solution obtained
and the best solutions produced in the experiment.


465


the removals and insertions take longer when more
requests are involved; thus, the comparison in Table 3
is not completely fair.
The complete parameter tuning resulted in the
following parameter vector +,-.ppworstwc
717273r<> = 9 325630 050 9997533
9130 10 0250 4. Our experiments also indicated
that it was possible to improve the performance of
the vehicle minimization algorithm by setting wc=
0 350 9999, while searching for solutions that serve
all requests. This corresponds to a higher start tem-
perature and a slower cooling rate. This indicates that
more diversification is needed when trying to mini-
mize the number of vehicles, compared to the situa-
tion where one just minimizes the traveled distance.
To tune the parameters we start from an initial
guess, and then tune one parameter at a time. When
all parameters are tuned, the process is repeated. In
this way the calibration order plays a minor role.
Although the parameter tuning is quite time con-
suming, it could easily be automated. In our subse-
quent papers (Ropke and Pisinger 2006; Pisinger and
Ropke 2005), where 11 variants of the vehicle rout-
ing problem are solved using the heuristic proposed
in this paper, we only retuned a few parameters and
obtained very convincing results, so it seems that a
complete tuning of the parameters only needs to be
done once.


4.3.3. LNS Configurations. This section evaluates


how the different removal and insertion heuristics
behave when used in an LNS heuristic. In most of
the test cases a simple LNS heuristic was used that
only involved one removal heuristic and one inser-
tion heuristic. Table 4 shows a summary of this
experiment.
The first six experiments aim at determining the
influence of the removal heuristic. We see that Shaw
removal performs best, the worst removal heuristic
is second, and the random removal heuristic gives
the worst performance. This is reassuring because
it shows that the two slightly more complicated
removal heuristics actually are better than the sim-
plest removal heuristic. These results also illustrate
that the removal heuristic can have a rather large
impact on the solution quality obtained, thus, experi-
menting with other removal heuristics would be inter-
esting and could prove beneficial.
The next eight experiments show the performance
of the insertion heuristics. Here we have chosen the
Shaw removal as the removal heuristic because it per-
formed best in the previous experiments. In these
experiments we see that all insertion heuristics per-
form quite well, and they are quite hard to distin-
guish from each other. Regret-3 and regret-4 coupled
with noise addition are slightly better than the rest,




466


<table>
 <tr>
  <th>Table 4</th>
  <th>LNS Heuristics</th>
  <th>to the Full Adaptive LNS with</th>
  <th>Weight Adjustment</th>
 </tr>
 <tr>
  <td>Conf.</td>
  <td>Shaw Rand Worst</td>
  <td>Reg-2 Reg-3</td>
  <td>Reg-m Noise Avg. gap (%)</td>
 </tr>
 <tr>
  <td>LNS 1</td>
  <td>•</td>
  <td>•</td>
  <td>27</td>
 </tr>
 <tr>
  <td>2</td>
  <td></td>
  <td></td>
  <td>•2 6</td>
 </tr>
 <tr>
  <td>3</td>
  <td>•</td>
  <td>•</td>
  <td>5 4</td>
 </tr>
 <tr>
  <td>4</td>
  <td>•</td>
  <td>•</td>
  <td>•3 2</td>
 </tr>
 <tr>
  <td>5</td>
  <td>•</td>
  <td>•</td>
  <td>2 0</td>
 </tr>
 <tr>
  <td>6</td>
  <td>•</td>
  <td>•</td>
  <td>• 1 6</td>
 </tr>
 <tr>
  <td>7</td>
  <td>•</td>
  <td>•</td>
  <td>2 </td>
 </tr>
 <tr>
  <td>8</td>
  <td>•</td>
  <td>•</td>
  <td>• 1 6</td>
 </tr>
 <tr>
  <td>9</td>
  <td>•</td>
  <td>•</td>
  <td>1 8</td>
 </tr>
 <tr>
  <td>10</td>
  <td>•</td>
  <td>•</td>
  <td>• 13</td>
 </tr>
 <tr>
  <td>11</td>
  <td>•</td>
  <td>•</td>
  <td>20</td>
 </tr>
 <tr>
  <td>12</td>
  <td>•</td>
  <td>•</td>
  <td>• 13</td>
 </tr>
 <tr>
  <td>13</td>
  <td>•</td>
  <td></td>
  <td>• 18</td>
 </tr>
 <tr>
  <td>14</td>
  <td>•</td>
  <td></td>
  <td>• • 17</td>
 </tr>
 <tr>
  <td>ALNS 15</td>
  <td>• • •</td>
  <td>• • • •</td>
  <td>•• 13</td>
 </tr>
</table>


Ropke and Pisinger: An Adaptive Large Neighborhood Search Heuristic
Transportation Science 40(4), pp. 455-472, © 2006 INFORMS


Notes. The first column shows if the configuration must be considered as an LNS or an ALNS heuristic. The second
column is the configuration number, Columns 3 to 5 indicate which removal heuristics were used. Columns 6 to 10
indicate which insertion heuristics were used. Column 11 states if noise was added to the objective function during
insertion of requests (in this case noise was added to the objective function in 50% of the insertions for the simple
Configurations 1-14 while in Configuration 15 the number of noise insertions was controlled by the adaptive method).
Column 12 shows the average performance of the different heuristics. As an example, in Configuration 4 we used
random removal together with the regret-2 insertion heuristic and we applied noise to the objective value. This resulted
in a set of solutions whose objective values on average were 3.2% above the best solutions found during the whole
experiment.


however. An observation that applies to all experi-
ments is that application of noise seems to help the
heuristic. It is interesting to note that the basic inser-
tion heuristic performs nearly as well as the regret
heuristics when used in an LNS framework. This is
surprising seen in the light of Table 2, where the basic
insertion heuristic performed particularly badly. This
observation may indicate that the LNS method is rel-
atively robust with respect to the insertion method
used.
The last row of the table shows the performance
of ALNS. As one can see, it is on par with the two
best simple approaches, but not better, which at first
may seem disappointing. The results show, however,
that the adaptive mechanism is able to find a sensi-
ble set of weights, and it is our hypothesis that the
ALNS heuristic is more robust than the simpler LNS
heuristics. That is, the simple configuration may fail
to produce good solutions on other types of problems,
while the ALNS heuristic continues to perform well.
One of the purposes of the experiments in §4.4 is to
confirm or disprove this hypothesis.


# 4.4. Results


This section provides computational experiments con-
ducted to test the performance of the heuristic. There
are three major objectives for this section:


1. To compare the ALNS heuristic to a simple LNS


heuristic that only contains one removal and one
insertion heuristic.


2. To determine if certain problem properties influ-


ence the (A)LNS heuristics ability to find good
solutions.


3. To compare the ALNS heuristic with state-of-the-


art PDPTW heuristics from the literature.
To clarify if the ALNS heuristic is worthwhile com-
pared to a simpler LNS heuristic, we are going to
show results for both the ALNS heuristic and the
best simple LNS heuristic from Table 4. Configura-
tion 12 was chosen as representative for the simple
LNS heuristics as it performed slightly better than
Configuration 10. In the following sections we refer to
the full and simple LNS heuristic as ALNS and LNS,
respectively.
All experiments were performed on a 1.5 GHz
Pentium IV PC with 256 MB internal memory, run-
ning Linux. The implemented algorithm measures
travel times and distances using double precision
floating point numbers. The parameter setting found
in §4.3.2 was used in all experiments unless otherwise
stated.


4.4.1. Data Sets. As the model considered in


this paper is quite complicated, it is hard to find
any benchmark instances that consider exactly the
same model and objective function. The benchmark
instances that come closest to the model considered
in this paper are the instances constructed by Nanry
and Barnes (2000) and the instances constructed by
Li and Lim (2001). Both data sets are single-depot
pickup and delivery problems with time windows,




Ropke and Pisinger: An Adaptive Large Neighborhood Search Heuristic
Transportation Science 40(4), pp. 455-472, © 2006 INFORMS


# Summary of Results Obtained on Li and Lim Instances (2001)


Table 5


Best known
solutions


Avg. gap (%)


Average time (s)


Number of
locations


Number of
problems


Fails


ALNS LNS ALNS LNS ALNS LNS ALNS LNS


50 0.19 0.50 49 55 0 0
15 0.72 1.41 305 314 0 0
6 2.36 4.29 585 752 0 0
5 0.93 3.20 1069 1470 0 0
5 1.73 3.27 2025 3051 0 2
4 2.26 4.22 2916 5252 0 1


100
200
400
600
800
1,000


56
60
60
60
60
58


52
49
52
54
46
47


Notes. The first column gives the problem size; the next column indicates the number of problems in the data set
of the particular size. The rest of the table consists of four major columns, each divided into two subcolumns, one
for the ALNS and one for LNS. The column Best known solutions indicates for how many problems the best known
solution was identified. The best known solution is either the solution reported by Li and Lim or the best solution
identified by the (A)LNS heuristics, depending on which is best. The next column indicates how far the average
solution is from best known solution. This number is averaged over all problems of a particular size. The next
column shows how long the heuristic on average spends to solve a problem. The last column shows the number
of times the heuristic failed to find a solution where all requests are served by the given number of vehicles in all
the attempts to solve a particular problem.


467


constructed from VRPTW problems. We are only
reporting results on the data set proposed by Li and
Lim, as the Nanry and Barnes instances are easy to
solve due to their size.
The problem considered by Li and Lim was sim-
pler than the one considered in this paper because:


(1) it did not contain multiple depots; (2) all requests


must be served; (3) all vehicles were assumed to be
able to serve all requests. When solving the Li and
Lim instances using the ALNS heuristic, we set  to
one and  to zero in our objective function. In §4.5
we minimize the number of vehicles as first priority
while in §4.4.2 we only minimize the distance driven.
In order to test all aspects of the model proposed
in this paper, we also introduce some new, randomly
generated instances. These instances are described
in §4.4.3.


# 4.4.2. Comparing ALNS and LNS Using the Li


and Lim Instances. This section compares the ALNS
and LNS heuristics using the benchmark instances
proposed by Li and Lim (2001). The data set contains
354 instances with between 100 and 1,000 locations.
The data set can be downloaded from SINTEF.
In this section we use the distance driven as our
objective even though vehicle minimization is the
standard primary objective for these instances. The
reason for this decision is that distance minimization
makes comparison of the heuristics easier, and dis-
tance minimization is the original objective of the pro-
posed heuristic. The number of vehicles available for
serving the requests is set to the minimum values
reported by Li and Lim in (2001) and on their Web
page, which unfortunately is no longer online.
The heuristics were applied 10 times to each
instance with 400 or fewer locations and 5 times


to each instance with more than 400 locations. The
experiments are summarized in Table 5.
The results show that the ALNS heuristic on all four
terms performs better than the LNS heuristic. One
also notices that the ALNS heuristic becomes even
more attractive as the problem size increases. It may
seem odd that the LNS heuristic spends more time
compared to the ALNS heuristic when they both per-
form the same number of LNS iterations. The reason
for this behavior is that the Shaw removal heuristic
used by the LNS heuristic is more time consuming
compared to the other two removal heuristics.


4.4.3. New Instances. This section provides re-


sults on randomly generated PDPTW instances that
contain features of the model that were not used in
the Li and Lim (2001) benchmark problems consid-
ered in §4.4.2. These features are: multiple depots,
routes with different start and end terminals, and spe-
cial requests that can only be served by a certain sub-
set of the vehicles. When solving these instances we
set ==1intheobjective function so that distance
and time are weighted equally in the objective func-
tion. We do not perform vehicle minimization because
the vehicles are inhomogeneous.
Three types of geographical distributions of re-
quests are considered: problems with locations dis-
tributed uniformly in the plane, problems with
locations distributed in 10 clusters, and problems
with 50% of the locations put in 10 clusters and
50% of the locations distributed uniformly. These
three types of problems were inspired by Solomon's
VRPTW benchmark problems (1987), and the prob-
lems are similar to the R, the C, and the RC Solomon
problems, respectively. We consider problems with 50,
100, 250, and 500 requests; all problems are multide-
pot problems. For each problem size we generated




468


Table 6


Number of
requests


50
100
250
500


# Summary of Results Obtained on New Instances


Best known
solutions


Avg. gap (%)


Average time (s)


Number of
problems


ALNS LNS ALNS LNS ALNS LNS


5 1.44 1.86
1 1.54 2.18
5 1.39 1.62
3 1.18 1.32


23
83
577
3805


34
142
1274
8146


12
12
12
12


8
11
7
9


Sum: 48 35 14 5.55 6.98 4488 9596
Notes. The captions of the table should be interpreted as in Table 5. The last
row sums each column. Notice that the size of the problems in this table is
given as number of requests and not the number of locations.


12 problems as we tried every combination of the
three problem features shown below:


• Route type: (1) A route starts and ends at the


same location, (2) a route starts and ends at different
locations.


• Request type: (1) All requests are normal


requests, (2) 50% of the requests are special requests.
The special requests can only be served by a subset of
the vehicles. In the test problems each special request
could only be served by between 30% to 60% of the
vehicles.


• Geographical distributions: (1) uniform, (2) clus-


tered, and (3) semiclustered.
The instances can be downloaded from www.diku.
dk/∼sropke. The heuristics were tested by applying
them to each of the 48 problems 10 times. Table 6
shows a summary of the results found. In the table we
list the number of problems for which the two heuris-
tics find the best known solution. The best known
solution is simply the best solution found throughout
this experiment.


<table>
 <tr>
  <th>Best known 2003 BH best ALNS best of 10 or 5 ALNS best</th>
 </tr>
 <tr>
  <td>Number of Number of Number of Number of Number of</td>
 </tr>
 <tr>
  <td>locations vehicles Dist. vehicles Dist. Avg. TTB Avg. time vehicles Dist. Avg. time vehicles Dist.</td>
 </tr>
 <tr>
  <td>100 402 58060 402 58062 68 3900 402 58060 66 402 56060</td>
 </tr>
 <tr>
  <td>200 615 178380 614 180358 772 3900 606 180931 264 606 180419</td>
 </tr>
 <tr>
  <td>400 1183 421215 1188 423636 2581 6000 1158 422201 881 1157 420396</td>
 </tr>
 <tr>
  <td>600 1699 873850 1718 879940 3376 6000 1679 863442 2221 1664 860898</td>
 </tr>
 <tr>
  <td>800 2213 1492200 2245 1480767 5878 8100 2208 1432078 3918 2181 1423063</td>
 </tr>
 <tr>
  <td>1,000 2698 2195755 2759 2225190 6174 8100 2652 2137034 5370 2646 2122922</td>
 </tr>
</table>


Ropke and Pisinger: An Adaptive Large Neighborhood Search Heuristic
Transportation Science 40(4), pp. 455-472, © 2006 INFORMS


We observe the same tendencies as in Table 5;
ALNS is still superior to LNS, but one notices that the
gap in solution quality between the two methods are
smaller for this set of instances while the difference in
running time is larger compared to the results on the
Li and Lim (2001) instances. One also notices that it
seems harder to solve small instances of this problem
class compared to the Li and Lim instances.
Table 8 summarizes how the problem features influ-
ence the average solution quality. These results show
that the clustered problems are the hardest to solve,
while the uniformly distributed instances are the
easiest. The results also indicate that special requests
make the problem slightly harder to solve. The route-
type experiments compare the situation where routes
start and end at the same location (the typical sit-
uation considered in the literature) to the situation
where each route starts and ends at different loca-
tions. Here we expect the last case to be the eas-
ier to solve, as we by having different start and end
positions for our routes gain information about the
area the route most likely should cover. The results in
Table 8 confirm these expectations.
In addition to investigating the question of how the
model features influence the average solution quality
obtained by the heuristics, we also want to know if
the presence of some features could make LNS behave
better than ALNS. For the considered features, the
answer is negative.


# 4.5. Comparison to Existing Heuristics


This section compares the ALNS heuristics to exist-
ing heuristics for the PDPTW. The comparison is per-
formed using the benchmark instances proposed by
Li and Lim (2001) that also were used in §4.4.2. When
PDPTW problems have been solved in the literature,


Comparison Between ALNS Heuristic and Existing Heuristics


Table 7


Notes. Each row in the table corresponds to a set of problems with the same number of locations. Each of these problem sets contain between 56 and
60 instances (see Table 9). The first column indicates the number of locations in each problem; the next two columns give the total number of vehicles used
and the total distance traveled in the previously best known solutions as listed on the SINTEF Web page in the summer of 2003. The next four columns show
information about the solutions obtained by Bent and Van Hentenryck's heuristic (2003a). The two columns Avg. TTB and Avg. time show the average time
needed to reach the best solution and the average time spent on each instance, respectively. Both columns report the time needed to perform one experiment
on one instance. The next three columns report the solutions obtained in the experiment with the ALNS heuristic where the heuristic was applied either 5or
10 times to each problem. The last two columns report the best solutions obtained in several experiments with our ALNS heuristic and with various parameter
settings. Note that Bent and Van Hentenryck in some cases have found slightly better results than reported on the SINTEF Web page in 2003. This is the reason
why the number of vehicles used by the BH heuristic for the 200 locations problems is smaller than in the best known solutions.




Ropke and Pisinger: An Adaptive Large Neighborhood Search Heuristic
Transportation Science 40(4), pp. 455-472, © 2006 INFORMS


Summary of the Influence of Certain Problem Features on the
Heuristic Solutions


Table 8


Feature


ALNS (%)


LNS (%)


Distribution: uniform 1.04
Distribution: clustered 1.89
Distribution: semiclustered 1.23
Normal requests 1.24
Special requests 1.54
Start of route = end of route 1.59
Start of route = end of route 1.19
Notes. The two columns correspond to
row shows the average solution quality
tion quality is defined as the average of
with a specific feature. To be more precise,
using the formula: q h = 1/Fi∈F
10j=1
where F is the set of instances with a specific
best known solution to instance i, and c ijh is the
jth experiment on instance i using heuristic h.


1. 

2. 


1.64


1. 

2. 


2.04


1.45


the two heuristic configurations. Each
for each feature. The average solu-
the average gap for all instances
the solution quality is calculated
1/10
c i j h -c
 i/c
 i
feature, c
 i is the cost of the
cost obtained in the


the primary objective has been to minimize the num-
ber of vehicles used, while the secondary objective
has been to minimize the traveled distance. For this
purpose we use the vehicle minimization algorithm
described in §3.7. The ALNS heuristic was applied
10 times to each instance with 200 or fewer locations
and 5 times to each instance with more than 200 loca-
tions. The experiments are summarized in Tables 7,
9, and 10. It should be noted that it was necessary to
decrease the w parameter and increase the c param-
eter when the instances with 1,000 locations were
solved to get reasonable solution quality. Apart from
that, the same parameter setting has been used for all
instances.
In the literature, four heuristics have been applied
to the benchmark problems: the heuristic by Li and


Comparison with Previously Best Known Solutions


Table 9


ALNS best of


<table>
 <tr>
  <th></th>
  <th>10 or 5</th>
  <th>ALNS best</th>
  <th></th>
  <th>its best solution</th>
  <th>toward the end of the search. The</th>
 </tr>
 <tr>
  <td>Number of Number of</td>
  <td></td>
  <td></td>
  <td></td>
  <td>BH heuristic</td>
  <td>tested on a 1.2 GHz Athlon pro-</td>
 </tr>
 <tr>
  <td>locations problems</td>
  <td>&lt;PB ≤PB</td>
  <td>&lt;PB ≤PB</td>
  <td></td>
  <td>cessor, and the</td>
  <td>running times of the two heuristics</td>
 </tr>
 <tr>
  <td>100 56</td>
  <td>0 54</td>
  <td>0 55</td>
  <td></td>
  <td>should</td>
  <td>be comparable (we believe that the</td>
 </tr>
 <tr>
  <td>200 60</td>
  <td>22 42</td>
  <td>27 57</td>
  <td></td>
  <td>Athlon</td>
  <td>is at most 20% slower than our com-</td>
 </tr>
 <tr>
  <td>400 60</td>
  <td>40 47</td>
  <td>41 55</td>
  <td></td>
  <td>puter). The</td>
  <td>show that overall the ALNS heuris-</td>
 </tr>
 <tr>
  <td></td>
  <td></td>
  <td></td>
  <td></td>
  <td>tic dominates</td>
  <td>BH heuristic, especially as the prob-</td>
 </tr>
 <tr>
  <td></td>
  <td></td>
  <td></td>
  <td></td>
  <td></td>
  <td></td>
 </tr>
 <tr>
  <td></td>
  <td></td>
  <td></td>
  <td></td>
  <td></td>
  <td></td>
 </tr>
 <tr>
  <td></td>
  <td></td>
  <td></td>
  <td></td>
  <td></td>
  <td></td>
 </tr>
 <tr>
  <td></td>
  <td></td>
  <td></td>
  <td></td>
  <td></td>
  <td>on</td>
 </tr>
 <tr>
  <td></td>
  <td></td>
  <td></td>
  <td></td>
  <td>best known</td>
  <td>and that the vehicle minimiza-</td>
 </tr>
</table>


Notes. The table is grouped by problem size. The first column shows
the problem size; the next column shows the number of problems
of that size. The next two columns give additional information about
ALNS
applied
many
previously best known solution. The columns ≤PB show how many
times the best solution found by ALNS was at least as good as the
previously best known solution. The last two columns show informa-
tion about the best solutions obtained during experimentation with
different parameter settings.


<table>
 <tr>
  <th>the experiment where the</th>
  <th>heuristic was</th>
  <th>5 or 10 times</th>
  <th></th>
  <th>The last two</th>
  <th>in Table 7 summarize the best</th>
 </tr>
 <tr>
  <td>to each instance. The columns</td>
  <td>&lt;PB report how</td>
  <td>times the best</td>
  <td></td>
  <td>results obtained</td>
  <td>using several experiments with dif-</td>
 </tr>
 <tr>
  <td>solution found by the ALNS</td>
  <td>heuristic was strictly</td>
  <td>better than the</td>
  <td></td>
  <td>ferent parameter</td>
  <td>settings, which show that the results</td>
 </tr>
</table>


Average Performance of the ALNS
Heuristic


Table 10


Number of
locations


Avg. number
of vehicles


Avg. dist.


403 58249
608 181707
1168 425817
1686 867930
2223 1432321
2677 2129032


100
200
400
600
800
1,000


Notes. The best solutions reported in Tables 7 and 9
were of course not obtained in all experiments. This table
shows the average number of vehicles and average dis-
tance traveled obtained. These numbers can be compared
to the figures in Table 7.


469


Lim (2001), the heuristic by Bent and Van Henten-
ryck (2003a), and two commercial heuristics: a heuris-
tic developed by SINTEF and a heuristic developed by
TetraSoft A/S. Detailed results for the last two heuris-
tics are not available, but some results obtained using
these heuristics can be found on a Web page main-
tained by SINTEF (http://www.sintef.no/static/am/
opti/projects/top/vrp/benchmarks.html). The heuris-
tic that has obtained the best overall solution qual-
ity so far is probably the one by Bent and Van Hen-
tenryck (2003a) (shortened to BH heuristic in the fol-
lowing), therefore the ALNS heuristic is compared to
this heuristic in Table 7. The complete results from the
BH heuristic can be found in Bent and Van Henten-
ryck (2003b). The results given for the BH heuristic
are the best obtained among 10 experiments (though
for the 100 location instances only five experiments
were performed). The Avg. TTB column shows the
average time needed for the BH heuristic to obtain
its best solution. For the ALNS heuristic we only list
the time used in total because this heuristic-because
of its simulated annealing component-usually finds
was
therefore
processor
results
the
lem sizes increase. It is also clear that the ALNS heuris-
tic is able to improve considerably
the previously
solutions
tion algorithm works very well despite its simplicity.
columns
obtained by ALNS actually can be improved even
further.
Table 9 compares the results obtained by ALNS
with the best known solutions from the literature. It




470


<table>
 <tr>
  <th>Table 11</th>
  <th>Best</th>
  <th>100</th>
  <th>Locations</th>
 </tr>
 <tr>
  <td></td>
  <td>R1</td>
  <td></td>
  <td>R2 C1 C2 RC1 RC2</td>
 </tr>
 <tr>
  <td>1</td>
  <td>165080</td>
  <td>4</td>
  <td>125323 10 82894 3 59156 14 170880 4 140694</td>
 </tr>
 <tr>
  <td>2</td>
  <td>148757</td>
  <td>3</td>
  <td>119767 10 82894 3 59156 12 155807 3 137427</td>
 </tr>
 <tr>
  <td>3</td>
  <td>129268</td>
  <td>3</td>
  <td>94940 9 103535 3 59117 11 125874 3 108907</td>
 </tr>
 <tr>
  <td>4</td>
  <td>9 101339</td>
  <td>2</td>
  <td>84905 9 86001 3 59060 10 112840 3 81866</td>
 </tr>
 <tr>
  <td>5</td>
  <td>137711</td>
  <td>3</td>
  <td>105402 10 82894 3 58888 13 163762 4 130220</td>
 </tr>
 <tr>
  <td>6</td>
  <td>125262</td>
  <td>3</td>
  <td>93163 10 82894 3 58849 11 142473 3 115903</td>
 </tr>
 <tr>
  <td>7</td>
  <td>111131</td>
  <td>2</td>
  <td>90306 10 82894 3 58829 11 123014 3 106205</td>
 </tr>
 <tr>
  <td>8</td>
  <td>9 96897</td>
  <td>2</td>
  <td>73485 10 82644 3 58832 10 114743 3 85276</td>
 </tr>
 <tr>
  <td>9</td>
  <td>120896</td>
  <td>3</td>
  <td>93059 9 100060</td>
 </tr>
 <tr>
  <td>10</td>
  <td>115935</td>
  <td>3</td>
  <td>96422</td>
 </tr>
 <tr>
  <td>11</td>
  <td>110890</td>
  <td>2</td>
  <td>91152</td>
 </tr>
 <tr>
  <td>12</td>
  <td>9 100377</td>
  <td></td>
  <td></td>
 </tr>
</table>


Ropke and Pisinger: An Adaptive Large Neighborhood Search Heuristic
Transportation Science 40(4), pp. 455-472, © 2006 INFORMS


Notes. The Li and Lim (2001) benchmark instances are divided into six sets: R1, R2, C1, C2, RC1, and RC2. Each
of the major columns corresponds to one of these sets; the column at the left gives the problem number. For each
problem instance we report the number of vehicles and the distance traveled in the best solution obtained during
experimentation. Bold numbers indicate best known solutions.


can be seen that ALNS improves more than half of
the solutions and achieves a solution that is at least as
good as the previously best known solution for 80%
of the problems.
The two aforementioned tables dealt only with the
best solutions found by the ALNS heuristic. Table 10
shows the average solution quality obtained by the
heuristic. These numbers can be compared to those
in Table 7. It is worth noticing that the average solu-
tion sometimes has a lower distance than the "best
of 10 or 5" solution in Table 7; this is the case in the
last row. This is possible because the heuristic finds
solutions that use more than the minimum number
of vehicles, and this usually makes solutions with
shorter distances possible.
Overall, one can conclude that the ALNS heuristic
must be considered as a state-of-the-art heuristic for
the PDPTW. The cost of the best solutions identified
during the experiments are listed in Tables 11 to 16.


# 4.6. Computational Tests Conclusion


In §4.4 we stated three objectives for our computa-
tional experiments. The tests fulfilled these objectives
as we saw that: (1) the adaptive LNS heuristic that
combines several removal and construction heuristics


<table>
 <tr>
  <th></th>
  <th></th>
  <th>R1</th>
  <th></th>
  <th>R2</th>
  <th></th>
  <th>C1</th>
  <th>C2</th>
  <th>RC1</th>
  <th>RC2</th>
 </tr>
 <tr>
  <td>1</td>
  <td>20</td>
  <td>481912</td>
  <td>5</td>
  <td>407310</td>
  <td>20</td>
  <td>270457</td>
  <td>6 193144</td>
  <td>19 360606</td>
  <td>6</td>
 </tr>
 <tr>
  <td>2</td>
  <td>17</td>
  <td>462121</td>
  <td>4</td>
  <td>379600</td>
  <td>19</td>
  <td>276456</td>
  <td>6 188140</td>
  <td>15 367480</td>
  <td>5</td>
 </tr>
 <tr>
  <td>3</td>
  <td>15</td>
  <td>361264</td>
  <td>4</td>
  <td>309836</td>
  <td>17</td>
  <td>312861</td>
  <td>6 184433</td>
  <td>13 317817</td>
  <td>4</td>
 </tr>
 <tr>
  <td>4</td>
  <td>10</td>
  <td>303738</td>
  <td>3</td>
  <td>248614</td>
  <td>17</td>
  <td>269341</td>
  <td>6 176712</td>
  <td>10 263182</td>
  <td>3</td>
 </tr>
 <tr>
  <td>5</td>
  <td>16</td>
  <td>476018</td>
  <td>4</td>
  <td>343839</td>
  <td>20</td>
  <td>270205</td>
  <td>6 189121</td>
  <td>16 371581</td>
  <td>5</td>
 </tr>
 <tr>
  <td>6</td>
  <td>14</td>
  <td>417824</td>
  <td>4</td>
  <td>320154</td>
  <td>20</td>
  <td>270104</td>
  <td>6 185778</td>
  <td>17 336866</td>
  <td>5</td>
 </tr>
 <tr>
  <td>7</td>
  <td>12</td>
  <td>355061</td>
  <td>3</td>
  <td>313505</td>
  <td>20</td>
  <td>270104</td>
  <td>6 185013</td>
  <td>14 366839</td>
  <td>4</td>
 </tr>
 <tr>
  <td>8</td>
  <td>9</td>
  <td>278453</td>
  <td>2</td>
  <td>255540</td>
  <td>20</td>
  <td>268983</td>
  <td>6 182434</td>
  <td>13 317455</td>
  <td>4</td>
 </tr>
 <tr>
  <td>9</td>
  <td>14</td>
  <td>435466</td>
  <td>3</td>
  <td>393049</td>
  <td>18</td>
  <td>272424</td>
  <td>6 185421</td>
  <td>13 322672</td>
  <td>4</td>
 </tr>
 <tr>
  <td>10</td>
  <td>11</td>
  <td>371416</td>
  <td>3</td>
  <td>334408</td>
  <td>17</td>
  <td>294349</td>
  <td>6 181745</td>
  <td>12 295129</td>
  <td>3</td>
 </tr>
</table>


displays superior performance compared to the sim-
ple LNS heuristic that only uses one insertion heuris-
tic and one removal heuristic; (2) certain problem
characteristics influence the performance of the LNS
heuristic, but we did not find that any characteristics
could make the LNS heuristic perform better than the
ALNS heuristic; (3) the LNS heuristic is indeed able
to find good-quality solutions in a reasonable amount
of time, and the heuristic outperforms previously pro-
posed heuristics.
The experiments also illustrate the importance of
testing heuristics on large sets of problem instances,
because the difference between LNS and ALNS only
really becomes apparent when we consider large
instances. Note that the problems that need to be
solved in the real world often have dimensions com-
parable to or greater than the biggest problems solved
in this paper.
Finally, the computational experiments performed
in §4.3.3 indicated that a simple LNS heuristic seems
to be more sensitive to the choices of removal heuris-
tic compared to the choices of insertion heuristics. It
would be interesting to see if this holds in general for
other problems as well.


Table 12


Best Results, 200 Locations




Ropke and Pisinger: An Adaptive Large Neighborhood Search Heuristic
Transportation Science 40(4), pp. 455-472, © 2006 INFORMS


<table>
 <tr>
  <th>Table</th>
  <th>Best Results, 400 Locations</th>
 </tr>
 <tr>
  <td></td>
  <td>R1 R2 C1 C2 RC1 RC2</td>
 </tr>
 <tr>
  <td>1<br/>2<br/>3<br/>4<br/>5<br/>6<br/>7<br/>8<br/>9<br/>10</td>
  <td>1063975 8 975846 40 715206 12 411633 36 912715 12 747101<br/>1001585 7 949664 38 801243 12 414429 31 834606 11 630336<br/>884046 6 811653 33 830894 12 443175 25 738740 9 543820<br/>674433 4 664978 30 687800 12 403800 19 583858 5 532243<br/>1059954 7 857484 40 715000 12 403063 33 877375 11 612013<br/>952545 6 799506 40 715402 12 390029 31 817790 9 647956<br/>820037 5 692861 40 714943 12 396251 29 799208 8 636126<br/>594644 4 544740 39 711116 12 384445 27 761343 7 592893<br/>988614 6 804320 36 745221 12 418893 26 801348 7 530353<br/>801662 5 790477 35 738713 12 382844 24 706573 6 576078</td>
 </tr>
 <tr>
  <td>Table 14</td>
  <td>Best Results, 600 Locations</td>
 </tr>
 <tr>
  <td></td>
  <td>R1 R2 C1 C2 RC1 RC2</td>
 </tr>
 <tr>
  <td>1 59</td>
  <td>2283865  11 2194530 60 1409564 19 797798 53 1792488 16 1481772</td>
 </tr>
 <tr>
  <td>2 45</td>
  <td>10 58 18 44 14</td>
 </tr>
 <tr>
  <td>3 37</td>
  <td>1807314 8 1560996 50 1468343 17 872830 36 1406031 10 1281267</td>
 </tr>
 <tr>
  <td>4 28</td>
  <td>1326971 6 1081945 47 1364803 17 804197 25 1095052 7 1057487</td>
 </tr>
 <tr>
  <td>5 38</td>
  <td>2256281 9 1956741 60 1408630 19 804737 47 1674255 14 1300952</td>
 </tr>
 <tr>
  <td>6 32</td>
  <td>2064102 8 1726296 60 1409079 19 809411 44 1689437 13 1264398</td>
 </tr>
 <tr>
  <td>7 25</td>
  <td>1716290 6 1581242 60 1408376 19 799818 39 1539487 11 1200765</td>
 </tr>
 <tr>
  <td>8 19</td>
  <td>1195759 5 1095090 59 1455427 18 757993 36 1515479 10 1216343</td>
 </tr>
 <tr>
  <td>9 32</td>
  <td>2142305 8 1879936 54 1470612 18 950100 35 1513424 9 1376801</td>
 </tr>
 <tr>
  <td>10 27</td>
  <td>1872313 7 1703463 53 1487930 17 801994 31 1392551 8 1201694</td>
 </tr>
 <tr>
  <td>Table 15</td>
  <td>Best Results, 800 Locations</td>
 </tr>
 <tr>
  <td></td>
  <td>R1 R2 C1 C2 RC1 RC2</td>
 </tr>
 <tr>
  <td>1 80</td>
  <td>3931592 15 3381690 80 2518438 24 1168706 67 3226895 20 2328940</td>
 </tr>
 <tr>
  <td>2 59</td>
  <td>3437037 12 3257597 78 2606217 24 1435892 57 2839539 18 2178662</td>
 </tr>
 <tr>
  <td>3 44</td>
  <td>2971809 10 2531053 65 2591845 24 1319829 50 2435436 16 1658631</td>
 </tr>
 <tr>
  <td>4 25</td>
  <td>2119765 7 1950642 60 2297088 23 1337682 35 1824191 12 1412205</td>
 </tr>
 <tr>
  <td>5 50</td>
  <td>3904606 12 3263429 80 2521122 25 1232980 61 3099548 18 2029292</td>
 </tr>
 <tr>
  <td>6 42</td>
  <td>3365950 10 2787080 80 2516425 24 1270287 58 2856861 16 2108857</td>
 </tr>
 <tr>
  <td>7 32</td>
  <td>2729419 8 2507785 80 2515838 25 1185586 54 2816441 15 1969596</td>
 </tr>
 <tr>
  <td>8 21</td>
  <td>1957021 5 1925679 78 2534845 24 1148288 49 2615065 13 1900933</td>
 </tr>
 <tr>
  <td>9 42</td>
  <td>10 73 24 47 12</td>
 </tr>
 <tr>
  <td>10 32</td>
  <td>3020086 9 2826524 71 2571212 24 1157858 42 2427152 10 1976678</td>
 </tr>
 <tr>
  <td>Table 16</td>
  <td>Best Results, 1,000 Locations</td>
 </tr>
 <tr>
  <td></td>
  <td>R1 R2 C1 C2 RC1 RC2</td>
 </tr>
 <tr>
  <td>1 100</td>
  <td>5690388 19 4542258 100 4248866 30 1687924 85 4870283 22 3507370</td>
 </tr>
 <tr>
  <td>2 80</td>
  <td>4965210 15 4782444 95 4387019 31 1898098 73 4513570 21 3093274</td>
 </tr>
 <tr>
  <td>3 54</td>
  <td>4212444 11 3989432 82 4263111 30 1777249 55 3547572 16 2840351</td>
 </tr>
 <tr>
  <td>4 28</td>
  <td>3213336 8 2831495 74 3944300 29 1808993 40 2774704 12 2308320</td>
 </tr>
 <tr>
  <td>5 61</td>
  <td>5913586 14 5320998 100 4247741 31 1713753 76 4981618 18 3471396</td>
 </tr>
 <tr>
  <td>6 50</td>
  <td>4863763 12 4379211 101 4283839 31 1719801 69 4446908 17 3148526</td>
 </tr>
 <tr>
  <td>7 37</td>
  <td>3893654 9 3672820 100 4285499 31 1911767 64 4141316 17 2963963</td>
 </tr>
 <tr>
  <td>8 26</td>
  <td>2945232 7 2627809 98 4295156 30 1701863 60 4059017 - -</td>
 </tr>
 <tr>
  <td>9 50</td>
  <td>5222315 13 4844749 92 4239198 31 1756595 57 3958785 - -</td>
 </tr>
 <tr>
  <td>10 40</td>
  <td>4621835 11 4415566 90 4243516 29 1742555 52 3619500 12 2940290</td>
 </tr>
</table>


471


Note. Two entries are missing as the corresponding problem instances no longer exist.




472


# 5. Conclusions


This paper presented an extension to the large neigh-
borhood search and the ruin-and-recreate heuristic
called adaptive LNS. The heuristic was tested on
the pickup and delivery problem with time windows
achieving good results in a reasonable amount of
time. The idea of combining several subheuristics in
the same search proved to be successful.
Because the proposed model is quite general, it
would be interesting to examine if the model and
heuristic can be used to solve other vehicle routing
problems. We are currently working on this topic, and
the results are very promising because the heuris-
tic has been able to discover new best solutions to
standard benchmarks for vehicle routing problems
with time windows, multidepot vehicle routing prob-
lems, and other vehicle routing problems (Pisinger
and Ropke 2005; Ropke and Pisinger 2006).
It would also be interesting to apply the ideas
presented in this paper to other combinatorial opti-
mization problems. The adaptive LNS framework is
easily applicable to most problems, taking advantage
of the numerous robust and fast construction heuris-
tics designed during the last decades for various opti-
mization problems.


# Acknowledgments


The authors wish to thank Jakob Birkedal Nielsen for help-
ful discussions during the development of the heuristic
presented in this paper and Emilie Danna for valuable
criticism of the paper. Furthermore, they would like to
thank the three anonymous referees for valuable comments
and corrections.


# References


Ahuja, R. K., O. Ergun, J. B. Orlin, A. P. Punnen. 2002. A survey of
very large scale neighborhood search techniques. Discrete Appl.
Math. 123 75-102.
Bent, R., P. Van Hentenryck. 2003a. A two-stage hybrid algorithm
for pickup and delivery vehicle routing problems with time
windows. Lecture Notes in Computer Science, Vol. 2833. Proc.
Internat. Conf. Constraint Programming (CP-2003), 123-137.
Bent, R., P. Van Hentenryck. 2003b. A two-stage hybrid algorithm
for pickup and delivery vehicle routing problem with time
windows. Appendix available at http://www.cs.brown.edu/
people/rbent/pickup-appendix.ps.
Bent, R., P. Van Hentenryck. 2004. A two-stage hybrid local search
for the vehicle routing problem with time windows. Transporta-
tion Sci. 38(4) 515-530.
Cordeau, J.-F., G. Laporte, A. Mercier. 2001. A unified tabu search
heuristic for vehicle routing problems with time windows.


J. Oper. Res. Soc. 52 928-936.


Cormen, T. H., C. E. Leiserson, R. L. Rivest, C. Stein. 2001. Intro-
duction to Algorithms, 2nd ed. MIT Press, Boston, MA.
Desaulniers, G., J. Desrosiers, A. Erdmann, M. M. Solomon,


1. Soumis. 2002. The VRP with pickup and delivery. P. Toth,


D. Vigo, eds. The Vehicle Routing Problem. SIAM Mono-


graphs on Discrete Mathematics and Applications, Vol. 9. SIAM,
Philadelphia, PA, 225-242.


Ropke and Pisinger: An Adaptive Large Neighborhood Search Heuristic
Transportation Science 40(4), pp. 455-472, © 2006 INFORMS


Dumas, Y., J. Desrosiers, F. Soumis. 1991. The pickup and delivery
problem with time windows. Eur. J. Oper. Res. 54 7-22.
Gendreau, M., F. Guertin, J.-Y. Potvin, R. Séguin. 1998. Neighbor-
hood search heuristics for a dynamic vehicle dispatching prob-
lem with pick-ups and deliveries. Tech. Rep. CRT-98-10, Centre
de Recherche sur les Transport, Universite de Montreal, Mon-
treal, Canada.
Lau, H. C., Z. Liang. 2001. Pickup and delivery with time win-
dows: Algorithms and test case generation. ICTAI-2001, 13th
IEEE Conf. Tools Artificial Intelligence, Dallas, TX, 333-340.
Li, H., A. Lim. 2001. A metaheuristic for the pickup and delivery
problem with time windows. ICTAI-2001, 13th IEEE Conf. Tools
Artificial Intelligence, Dallas, TX, 160-170.
Lim, H., A. Lim, B. Rodrigues. 2002. Solving the pick up and deliv-
ery problem with time windows using "squeaky wheel" opti-
mization with local search. Technical Report 2002, Paper 7-
2002, Singapore Management University, Singapore.
Martello, S., P. Toth. 1981. An algorithm for the generalized assign-
ment problem. J. P. Brans, ed. Operational Research '81. North-
Holland, New York.
Mladenovi´c, N., P. Hansen. 1997. Variable neighborhood search.
Comput. Oper. Res. 24(11) 1097-1100.
Nanry, W. P., J. W. Barnes. 2000. Solving the pickup and delivery
problem with time windows using reactive tabu search. Trans-
portation Res. Part B 34 107-121.
Pisinger, D., S. Ropke. 2005. A general heuristic for vehicle
routing problems. Comput. Oper. Res. Forthcoming. http://
www.sciencedirect.com
Potvin, J.-Y., J.-M. Rousseau. 1993. A parallel route building algo-
rithm for the vehicle routing and scheduling problem with
time windows. Eur. J. Oper. Res. 66 331-340.
Ropke, S. 2002. Heuristics for the multi-vehicle pickup and deliv-
ery problem with time windows. Masters Thesis 01-11-8,
Department of Computer Science, University of Copenhagen,
Denmark.
Ropke, S., D. Pisinger. 2004. An adaptive large neighborhood search
heuristic for the pickup and delivery problem with time win-
dows. Technical report, Department of Computer Science, Uni-
versity of Copenhagen, Denmark.
Ropke, S., D. Pisinger. 2006. A unified heuristic for a large class of
vehicle routing problems with backhauls. Eur. J. Oper. Res. 171
750-775.
Shaw, P. 1997. A new local search algorithm providing high quality
solutions to vehicle routing problems. Technical report, Depart-
ment of Computer Science, University of Strathclyde, Scotland.
Shaw, P. 1998. Using constraint programming and local search
methods to solve vehicle routing problems. Proc. CP-98 Fourth
Internat. Conf. Principles Practice Constraint Programming.
Schrimpf, G., J. Schneider, H. Stamm-Wilbrandt, G. Dueck. 2000.
Record breaking optimization results using the ruin and recre-
ate principle. J. Comput. Phys. 159 139-171.
Sigurd, M., D. Pisinger, M. Sig. 2004. The pickup and delivery prob-
lem with time windows and precedences. Transportation Sci. 38
197-209.
SINTEF. Vehicle routing and travelling salesperson problems.
http://www.sintef.no/static/am/opti/projects/top/vrp/
benchmarks.html.
Solomon, M. M. 1987. Algorithms for the vehicle routing and
scheduling problems with time window constraints. Oper. Res.
35 254-265.
Toth, P., D. Vigo. 2002. An overview of vehicle routing problems.


P. Toth, D. Vigo, eds. The Vehicle Routing Problem. SIAM Mono-


graphs on Discrete Mathematics and Applications, Vol. 9. SIAM,
Philadelphia, PA, 1-26.
Trick, M. A. 1992. A linear relaxation heuristic for the generalized
assignment problem. Naval Res. Logist. 39 137-152.
Xu, H., Z.-L. Chen, S. Rajagopal, S. Arunapuram. 2003. Solving a
practical pickup and delivery problem. Transportation Sci. 37
347-364.


