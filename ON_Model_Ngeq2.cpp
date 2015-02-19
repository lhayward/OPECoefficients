/**********************************************************************************************
************************************ OPE COEFFICIENTS CODE ************************************
***********************************************************************************************
* Lauren E. Hayward Sierens
***********************************************************************************************
* File:   ON_Model_Ngeq2.cpp
**********************************************************************************************/

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <typeinfo>
#include <vector>
#include "FileReading.h"
#include "ON_Model_Ngeq2.h"

//typdef needed because uint is a return types:
typedef ON_Model_Ngeq2::uint uint;

/******* ON_Model_Ngeq2(uint spinDim, std::ifstream* fin, std::string outFileName, ... ********
********                Hyperrectangle* lattice, MTRand* randomGen)                    ********
********                                 (constructor)                                 *******/
ON_Model_Ngeq2::ON_Model_Ngeq2(uint spinDim, std::ifstream* fin, std::string outFileName, 
                               Hyperrectangle* lattice, MTRand &randomGen)
  : ON_Model(fin, outFileName, lattice)
{
  std::cout.precision(15);
  
  spinDim_ = spinDim;
  spins_ = new VectorSpins(N_, spinDim_);
  randomizeLattice(randomGen);
}

/******************************* ~ON_Model_Ngeq2() (destructor) ******************************/
ON_Model_Ngeq2::~ON_Model_Ngeq2()
{
  //delete the VectorSpins object:
  if( spins_ != NULL )
  { delete spins_; }
  spins_ = NULL;
} //destructor

/**************************************** flipCluster *****************************************
* This function flips all spins in the passed cluster by using the passed vector r.
**********************************************************************************************/
void ON_Model_Ngeq2::flipCluster(std::vector<uint> &cluster, Vector_NDim* r)
{
  uint clustSize = (uint)cluster.size();
  
  for( uint i=0; i<clustSize; i++ )
  { spins_->getSpin(cluster[i])->reflectOverUnitVecAndNormalize(r); }
} //flipCluster

/*********************************** getClusterOnSiteEnergy ***********************************
* This function calculates the onsite (non-interacting) part of the energy corresponding to 
* the spins in the passed cluster.
**********************************************************************************************/
double ON_Model_Ngeq2::getClusterOnSiteEnergy(std::vector<uint> &cluster)
{
  uint         clustSize = (uint)cluster.size();
  uint         latticeSite;
  double       energyh = 0;
  Vector_NDim* currSpin;
  
  for( uint i=0; i<clustSize; i++ )
  {
    latticeSite =  cluster[i];
    currSpin    =  spins_->getSpin(latticeSite);
    energyh     += currSpin->v_[0];    
  } //for loop
  
  return (-1.0)*h_*energyh;
} //getClusterOnSiteEnergy

/**************************************** getEnergy() ****************************************/
double ON_Model_Ngeq2::getEnergy()
{
  double       energyJ       = 0;
  double       energyh       = 0;
  Vector_NDim* currSpin;
  Vector_NDim* neighbour;
  
  //nearest neighbour term:
  for( uint i=0; i<N_; i++ )
  { 
    currSpin    = spins_->getSpin(i);
    for( uint j=0; j<D_; j++ )
    {
      neighbour =  spins_->getSpin( hrect_->getNeighbour(i,j) ); //nearest neighbour along 
                                                                 //j direction
      energyJ   += currSpin->dot( neighbour );
    } //j
  } //i
  
  //field term:
  for( uint i=0; i<N_; i++ )
  { 
    currSpin =  spins_->getSpin(i);
    energyh  += currSpin->v_[0]; 
  }
  
  return ((-1.0*J_*energyJ) + (-1.0*h_*energyh)) ;
} //getEnergy()

/************************************* getMagnetization() ************************************/
Vector_NDim* ON_Model_Ngeq2::getMagnetization()
{
  Vector_NDim* mag = new Vector_NDim( spinDim_,0 );
  
  for( uint i=0; i<N_; i++ )
  { mag->add( spins_->getSpin(i) ); }
  
  return mag;
}

/******************************* localUpdate(MTRand* randomGen) ******************************/
void ON_Model_Ngeq2::localUpdate(MTRand &randomGen)
{ 
  uint         latticeSite; //randomly selected spin location
  double       deltaE;
  Vector_NDim* spin_old = NULL;  //previous state of spin (at randomly selected lattice site)
  Vector_NDim* spin_new = NULL;  //new proposed value for spin
  Vector_NDim  nnSum(spinDim_, 0);
  
  //randomly generate a new spin:
  spin_new = new Vector_NDim(spinDim_, randomGen);
  
  //randomly select a spin on the lattice:
  latticeSite = randomGen.randInt(N_-1);
  spin_old    = spins_->getSpin(latticeSite);
  
  //loop to calculate the nearest neighbour sum:
  for( uint i=0; i<D_; i++ )
  { 
    nnSum.add( spins_->getSpin( hrect_->getNeighbour( latticeSite, i    ) ) ); 
    nnSum.add( spins_->getSpin( hrect_->getNeighbour( latticeSite, i+D_ ) ) );
  }
  
  //calculate the energy change for the proposed move:
  deltaE = -1.0*J_*( nnSum.dot(spin_new) - nnSum.dot(spin_old) )
           - h_*( spin_new->v_[0] - spin_old->v_[0] );
  
  //if the move is accepted:
  if( deltaE<=0 || randomGen.randDblExc() < exp(-deltaE/T_) )
  { 
    //delete the vector storing the old state of the spin:
    if(spin_old!=NULL)
    { delete spin_old; }
    spin_old = NULL;
    
    spins_->setSpin( latticeSite, spin_new );
    numAccept_local_++;
  }
  //otherwise, the move is rejected:
  else
  {
    //delete the vector storing the rejected new state of the spin:
    if(spin_new!=NULL)
    { delete spin_new; }
    spin_new = NULL;  
  }
}

/************************************* makeMeasurement() *************************************/
void ON_Model_Ngeq2::makeMeasurement()
{
  double            energyPerSpin = getEnergy()/(1.0*N_);
  
  measures.accumulate( "E",   energyPerSpin ) ;
  measures.accumulate( "ESq", pow(energyPerSpin,2) );
}

/**************************************** printSpins() ***************************************/
void ON_Model_Ngeq2::printSpins()
{ spins_->print(); }

/**************************** randomizeLattice(MTRand* randomGen) ****************************/
void ON_Model_Ngeq2::randomizeLattice(MTRand &randomGen)
{ 
  spins_->randomize(randomGen);
}

/********************************** sweep(MTRand* randomGen) *********************************/
void ON_Model_Ngeq2::sweep(MTRand &randomGen, bool pr)
{ 
  uint N1 = N_/2;     //number of local updates before Wolff step
  uint N2 = N_ - N1;  //number of local updates after Wolff step
  
  for( uint i=0; i<N1; i++ )
  { localUpdate(randomGen); }
  
  wolffUpdate(randomGen, 0, (spinDim_-1), pr);
  
  for( uint i=0; i<N2; i++ )
  { localUpdate(randomGen); }
}

/******************************** uintPower(int base, int exp) *******************************/
uint ON_Model_Ngeq2::uintPower(uint base, uint exp)
{
  uint result = 1;
  for(uint i=1; i<=exp; i++)
  { result *= base; } 
  
  return result;
} //uintPower method

/******************************* wolffUpdate(MTRand* randomGen) ******************************/
void ON_Model_Ngeq2::wolffUpdate(MTRand &randomGen, uint start, uint end, bool pr)
{
  uint               latticeSite;
  uint               neighSite;
  uint               clustSize;
  double             PAdd;
  double             exponent;  //exponent for PAdd
  double             onsiteEnergy_initial;
  double             onsiteEnergy_final;
  double             onsiteEnergy_diff;
  double             PAcceptCluster;
  double             rDotRef;
  Vector_NDim*       r = new Vector_NDim(spinDim_,randomGen, start, end);
  Vector_NDim*       reflectedSpin = NULL;
  std::vector<uint>  buffer;  //indices of spins to try to add to cluster (will loop until 
                              //buffer is empty)
  std::vector<uint>  cluster; //vector storing the indices of spins in the cluster
  
  buffer.reserve(N_);
  cluster.reserve(N_);
  
  latticeSite = randomGen.randInt(N_-1);
  //flipSpin(latticeSite, r);  //don't flip yet for efficiency
  inCluster_[latticeSite] = 1;
  cluster.push_back(latticeSite);
  buffer.push_back(latticeSite);
  
  while( !buffer.empty() )
  {
    latticeSite = buffer.back();
    buffer.pop_back();
    
    //Note: the spin at site 'latticeSite' is not flipped yet so we have to consider the
    //      energy difference that would result if it were already flipped:
    reflectedSpin = spins_->getSpin(latticeSite)->getReflectionAndNormalize(r);
    rDotRef       = r->dot(reflectedSpin);
    
    for( uint i=0; i<(2*D_); i++ )
    {
      neighSite = hrect_->getNeighbour( latticeSite, i );
      
      //define the exponent based on whether the bond is in the xy-plane or in z-direction:
      exponent = (2.0*J_/T_)*rDotRef*( r->dot(spins_->getSpin(neighSite)) );
      
      if (exponent < 0 )
      { 
        PAdd = 1.0 - exp(exponent);
        if( !( inCluster_[ neighSite ] ) && (randomGen.randDblExc() < PAdd) )
        { 
          //flipSpin(neighbours[site][i],r); 
          inCluster_[ neighSite ] = 1;
          cluster.push_back( neighSite );
          buffer.push_back( neighSite );
        }
      }
    }  //for loop over neighbours
    
    if(reflectedSpin!=NULL)
    { delete reflectedSpin; }
    reflectedSpin = NULL;
    
  } //while loop for buffer
  
  
  //flip the cluster if it is accepted:
  onsiteEnergy_initial = getClusterOnSiteEnergy(cluster);
  flipCluster(cluster, r); //flip in order to calculate the final energy
  onsiteEnergy_final = getClusterOnSiteEnergy(cluster);
  onsiteEnergy_diff = onsiteEnergy_final - onsiteEnergy_initial;
  
  if( pr || writeClusts_ )
  { 
    clustSize = cluster.size(); 
    if( writeClusts_ )
    { clustSizes_[clustSize-1]++; }
  }
  
  //If the onsite energy diff. is negative, accept the cluster move. If the onsite energy 
  //diff. is positive, accept the cluster move with probability exp^(-beta*onsiteEnery_diff).
  //Note: cluster is already flipped currently, so this is a check to see if we need to flip 
  //      it back:
  if(pr)
  { r->print(); }
  if( onsiteEnergy_diff > 0 )
  {
    PAcceptCluster = exp(-1.0*onsiteEnergy_diff/T_);
    if(pr)
    {
      std::cout << "  PAccept = " << PAcceptCluster << std::endl;
      std::cout << "  size = " << clustSize << std::endl << std::endl;
    }
    //check if we need to flip the cluster back to its original orientation:
    if( randomGen.randDblExc() >= PAcceptCluster )
    { 
      flipCluster(cluster, r);
      if( writeClusts_ )
      { clustSizes_rejected_[clustSize-1]++; }
    }
    else
    { 
      numAccept_clust_++;
      if( writeClusts_ )
      { clustSizes_accepted_[clustSize-1]++; }
    }
  }
  else 
  {
    numAccept_clust_++;
    if( writeClusts_ )
    { clustSizes_accepted_[clustSize-1]++; }
    
    if(pr)
    {
      std::cout << "  onsite <= 0" << std::endl;
      std::cout << "  size = " << clustSize << std::endl << std::endl;
    }
  }
  
  clearCluster(cluster);
  
  if(r!=NULL)
  { delete r; }
  r = NULL;
} //wolffUpdate()

/******************** writeBin(int binNum, int numMeas, int sweepsPerMeas) *******************/
void ON_Model_Ngeq2::writeBin(int binNum, int numMeas, int sweepsPerMeas)
{
  //Note: the following two measurements will be divided by numMeas in the call to
  //writeAverages() such that acceptance rates are written to file
  measures.accumulate( "AccRate_local", numAccept_local_/(1.0*N_*sweepsPerMeas) );
  measures.accumulate( "AccRate_clust", numAccept_clust_/(1.0*N_*sweepsPerMeas) );
  
  //if this is the first bin being written to file, then also write a line of text explaining
  //each column:
  if( binNum == 1)
  {
    fout << "# L \t T \t binNum";
    measures.writeMeasNames(&fout);
    fout << std::endl;
  }
  fout << L_[0] << '\t' << T_ << '\t' << binNum;
  measures.writeAverages(&fout, numMeas);
  fout << std::endl;
}

/************************* writeClustHistoData(std::string fileName) *************************/
void ON_Model_Ngeq2::writeClustHistoData(std::string fileName)
{
  std::ofstream fout_clust;
  
  if( writeClusts_ )
  {
    fout_clust.open(fileName.c_str());
    fout_clust.precision(15);
  
    fout_clust << "#T \t clustSize \t num_generated \t num_accepted \t num_rejected" << std::endl;
    for( uint i=0; i<N_; i++ )
    { 
      fout_clust << T_ << '\t' << (i+1) << '\t' << clustSizes_[i] << '\t' << clustSizes_accepted_[i] 
                 << '\t' << clustSizes_rejected_[i] << std::endl;
    }
    fout_clust.close();
  }
}

/************************************* zeroMeasurements() ************************************/
void ON_Model_Ngeq2::zeroMeasurements()
{ 
  ON_Model::zeroMeasurements();
  numAccept_local_ = 0;
  numAccept_clust_ = 0;
}