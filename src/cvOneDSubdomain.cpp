//
//  Subdomain.cxx - Source for a class to contain the discretization of 
//  ~~~~~~~~~~~~~   of the Geometry.
//
//  This class will contain the discretization of the geometry.
//
//  History:
//  Aug. 06, 2003, I. Vignon
//      added the Wave boundary condition
//  Mar. 26, 2003, I. Vignon
//      added the RCR boundary condition
//  Oct., 2000 J.Wan
//      added the Pressure wave boundary condition.
//  May 1999, J.Wan, B.Steele, G.R.Feijoo, S.A.Spicer and S.Strohband
//      Creation of file, 
//

# include "cvOneDGlobal.h"
# include "cvOneDSubdomain.h"
# include "cvOneDMaterialManager.h"

const double PI = 4.0 * atan(1.0);//IV 080703

using namespace std;

cvOneDSubdomain::cvOneDSubdomain(){
  mat = NULL;
  pressureWave = NULL;
  pressureTime = NULL;
  resistanceWave = NULL;
  resistanceTime = NULL;
  numPressurePts = 0;
  impedance   =  NULL;
  impedancePressure = NULL;
  //  impedanceFlow = NULL;
  numImpedancePts =0;
  // Coronary boundarycondition kimhj 09032005
  PressLVWave=NULL;
  PressLVTime=NULL;
  numPressLVPts=0;
  K = new double[1000];
  for(int i=0;i<1000;i++){
      K[i]=0.0;
  }
  branchAngle = 90.0;
  //for wave BC added IV 080703
  numWavePts =0;
  eigValWave = NULL;
  WaveEndBC = 0.0;
  WaveIni = 0.0;
  MemEWave = 0.0;
  MemEIWave = 0.0;
  MemEIw = NULL;
  dMemEIw = NULL;
  MemEISpWave = NULL;
  MemEISmWave = NULL;
  MemEIS2pWave = NULL;
  MemEIS2mWave = NULL;
  MemEIw1 = NULL;
  MemEIw2 = NULL;
}

cvOneDSubdomain::~cvOneDSubdomain(){
  if(connectivities != NULL)  delete [] connectivities;
  if(nodes != NULL)      delete [] nodes;
  if(pressureWave != NULL)  delete [] pressureWave;
  if(pressureTime != NULL)  delete [] pressureTime;
  if(resistanceWave != NULL)  delete [] resistanceWave;
  if(resistanceTime != NULL)  delete [] resistanceTime;
  if(impedance != NULL)    delete [] impedance;
  if(impedancePressure != NULL) delete [] impedancePressure;
  // if(impedanceFlow != NULL) delete [] impedanceFlow;
  // if(impedanceDpDs!= NULL) delete [] impedanceDpDs;
  if(eigValWave != NULL) delete [] eigValWave;//IV 080703
  if(MemEIw != NULL) delete [] MemEIw;//IV 080703
  if(dMemEIw != NULL) delete [] dMemEIw;//IV 080703
  if(MemEISpWave != NULL) delete [] MemEISpWave;//IV 080703
  if(MemEISmWave != NULL) delete [] MemEISmWave;//IV 080703
  if(MemEIS2pWave != NULL) delete [] MemEIS2pWave;//IV 091003
  if(MemEIS2mWave != NULL) delete [] MemEIS2mWave;//IV 091003
  if(MemEIw1 != NULL) delete [] MemEIw1;//IV 080703
  if(MemEIw2 != NULL) delete [] MemEIw2;//IV 080703
  if(presslv != NULL) delete [] presslv;//kimhj 09032005
  if(PressLVWave != NULL)  delete [] PressLVWave;//kimhj 09032005
  if(PressLVTime != NULL)  delete [] PressLVTime;//kimhj 09032005
}

void cvOneDSubdomain::SetInitInletS(double So){
  S_initial = So;  
}

void cvOneDSubdomain::SetInitOutletS(double Sn){
  S_final = Sn;
}

void cvOneDSubdomain::SetInitialFlow(double Qo){
  Q_initial = Qo;
}

void cvOneDSubdomain::SetInitialPressure(double Po){
  P_initial = Po;
}

void cvOneDSubdomain::SetInitialdFlowdT(double dQ0dT){
  //dQ_dT_initial = dQ0dT;
  dQ_dT_initial = 0;
}

void cvOneDSubdomain::SetupMaterial(int matID){
  mat = cvOneDGlobal::gMaterialManager->GetNewInstance(matID);
  mat->SetAreas_and_length(S_initial, S_final, fabs(z_out - z_in));
}

void cvOneDSubdomain::SetBoundValue(double boundV){
  switch(boundType){
    case BoundCondTypeScope::PRESSURE:
      boundValue = mat->GetArea(boundV*1333.2237, fabs(z_out-z_in));
      break;
    default:
      boundValue = boundV;
      break;
  }
}

double cvOneDSubdomain::GetInitInletS(void) {return S_initial;}
double cvOneDSubdomain::GetInitOutletS(void) {return S_final;}
double cvOneDSubdomain::GetInitialFlow(void) {return Q_initial;}
double cvOneDSubdomain::GetInitialPressure(void) {return P_initial;}

// kimhj added for coronary boundary conditions kimhj 09022005
double cvOneDSubdomain::GetInitialdFlowdT(void){ 
  return dQ_dT_initial; 
  // return 0;
}

void cvOneDSubdomain::SetNumberOfNodes(long nNodes){
  numberOfNodes = nNodes;
}

void cvOneDSubdomain::SetNumberOfElements(long nElements){
  numberOfElements = nElements;
}

void cvOneDSubdomain::SetMeshType(MeshType mType){
  meshType = mType;
}

void cvOneDSubdomain::Init(double x0, double xL){
  z_in = x0;
  z_out = xL;
  char errStr[256];
  nodes = new double[numberOfNodes];
  connectivities = new long[ 2 * numberOfElements];
  assert( nodes != 0 && connectivities != 0);

  // Only handle uniform mesh for now...
  if(meshType != MeshTypeScope::UNIFORM){
    cvOneDError::setErrorNumber(ErrorTypeScope::UNSUPPORTED);
    strcpy(errStr,"In Subdomain::Init(...), Unsuported mesh type requested, continuing with a uniform mesh.");
    cvOneDError::setErrorString(errStr);
    cvOneDError::CallErrorHandler();
  }
  
  // Set the mesh to be uniform
  double h = (xL - x0) / static_cast<double>(numberOfElements);
  
  for( long i = 0; i < numberOfNodes; i++){
    nodes[i] = x0 + i * h;
  }
    
  long nd = 0;
  for( long element = 0; element < numberOfElements; element++, nd++){
    connectivities[ 2 * element    ] = nd;
    connectivities[ 2 * element + 1] = nd + 1;
  }

  finiteElement = new cvOneDFiniteElement();
  assert( finiteElement != 0);
}

long cvOneDSubdomain::GetNumberOfNodes()const{
  return numberOfNodes;
}

long cvOneDSubdomain::GetNumberOfElements()const{
  return numberOfElements;
}

void cvOneDSubdomain::GetConnectivity(long element, long* conn)const{
  conn[0] = connectivities[ 2 * element    ] + global1stNodeID;
  conn[1] = connectivities[ 2 * element + 1] + global1stNodeID;
}

void cvOneDSubdomain::GetNodes( long element, double* nd)const{
  nd[0] = nodes[ connectivities[2*element]];
  nd[1] = nodes[ connectivities[2*element+1]];
}

cvOneDFiniteElement* cvOneDSubdomain::GetElement(long element)const{
  long conn[2];
  double nd[2];
  GetConnectivity( element, conn);
  GetNodes( element, nd);

  finiteElement->Set( nd, conn);

  return finiteElement;
}

double cvOneDSubdomain::GetNodalCoordinate( long node)const{
  return nodes[node];
}

void cvOneDSubdomain::SetBoundPresWave(double *time, double *pres, int num){
  int i ;
  pressureWave = new double[num];
  pressureTime = new double[num];
  for(i = 0; i < num; i++){
      pressureTime[i] = time[i];
      pressureWave[i] = pres[i]*1333.2237;
        // fprintf(stdout,"pressureTime[%i]: %le pressureWave[%i]: %le\n", i,pressureTime[i], i, pressureWave[i]);
  }
  numPressurePts = num;
}

void cvOneDSubdomain::SetBoundResistanceWave(double *time, double *resist, int num){
  int i ;
  resistanceWave = new double[num];
  resistanceTime = new double[num];
  for(i = 0; i < num; i++){
      resistanceTime[i] = time[i];
      resistanceWave[i] = resist[i];
  }
  numPressurePts = num;
}

void cvOneDSubdomain::SetBoundImpedanceValues(double *h, int num){
  numImpedancePts = num;
  impedance     = new double[num];
  impedancePressure = new double[num];
  impedanceTime = 0.0;
  for(int i=0 ; i < num ; i++){
    impedance[i] = h[i];
    impedancePressure[i] =  mat->GetReferencePressure();
    // fprintf(stdout,"impedance[%i]: %le  pressure[%i]: %le\n",i,impedance[i],i,impedancePressure[i]);
  }
}

double cvOneDSubdomain::getBoundCoronaryValues(double currentTime){
  if(PressLVTime == NULL || PressLVWave == NULL){ 
      cout << "ERROR: LV pressure information is not prescribed"<< endl; 
      exit(1); 
  } 
  // flow rate is assumed to be periodic
  double cycleTime = PressLVTime[numPressLVPts-1];
  double correctedTime = currentTime - static_cast<long>(currentTime / cycleTime) * cycleTime; 
  double presslv=0.;
  if(correctedTime >= 0 && correctedTime <= PressLVTime[0]){
    double xi = (correctedTime - PressLVTime[numPressLVPts-1]) / (PressLVTime[0] - PressLVTime[numPressLVPts-1]); 
    presslv = (PressLVWave[numPressLVPts-1] + xi * (PressLVWave[0] - PressLVWave[numPressLVPts-1])); 
  }else{
    int ptr = 0; 
    bool wasFound = false; 
    while( !wasFound){ 
      if(correctedTime >= PressLVTime[ptr] && correctedTime <= PressLVTime[ptr+1]){
        wasFound = true; 
      }else{
        ptr++; 
      }
    }
 
    // linear interpolation between values 
    double xi = (correctedTime - PressLVTime[ptr]) / (PressLVTime[ptr+1] - PressLVTime[ptr]); 

    presslv = (PressLVWave[ptr] + xi * (PressLVWave[ptr+1] - PressLVWave[ptr])); 
  }
  return presslv*1333.2237;
}

// Added kimhj 09022005
void cvOneDSubdomain::SetBoundCoronaryValues(double *time, double *p_lv, int num){
  
  int numpts=num;  
  corTime=0.0;
  //a=Ra1*Ra2*Ca*Cc;
  //b=Ca*Ra1+Cc*(Ra1+Ra2);
  //expo1=(-b+sqrt(b*b-4*a))/2/a;
  //expo2=(-b-sqrt(b*b-4*a))/2/a;
  if (time[0]< 0.0) {Ra1=p_lv[0];}
  if (time[1]< 0.0) {Ra2=p_lv[1];}
  if (time[2]< 0.0) {Ca=p_lv[2];}
  if (time[3]< 0.0) {Cc=p_lv[3];}
  if (time[4]< 0.0) {Rv1=p_lv[4];}
  if (time[5]< 0.0) {Rv2=p_lv[5];}
  if (time[6]< 0.0) {
    fprintf(stdout, "Wrong file format");
    exit(1);
  }
  numpts=num-6;
  numPressLVPts=numpts;
  PressLVWave=new double[numpts];
  PressLVTime=new double[numpts];
  for (int i=0;i<numpts;i++){
    PressLVTime[i]=time[i+6];
    PressLVWave[i]=p_lv[i+6];
    fprintf(stdout,"Time[%i]: %le Pressure_LV[%i]: %le\n",i, PressLVTime[i], i,PressLVWave[i]);  
  }
  p0COR=1;
  p1COR=Ra2*Ca+(Rv1+Rv2)*(Ca+Cc);
  p2COR=Ca*Cc*Ra2*(Rv1+Rv2);
  q0COR=Ra1+Ra2+Rv1+Rv2;
  q1COR=Ra1*Ca*(Ra2+Rv1+Rv2)+Cc*(Rv1+Rv2)*(Ra1+Ra2);
  q2COR=Ca*Cc*Ra1*Ra2*(Rv1+Rv2);
  b0COR=0;
  b1COR=Cc*(Rv1+Rv2);
  detCOR=sqrt(q1COR*q1COR-4*q0COR*q2COR);
  expo2COR=-(q1COR+detCOR)/2/q2COR;
  expo1COR=q0COR/q2COR/expo2COR;
  CoefZ1=(p0COR+p1COR*expo1COR+p2COR*expo1COR*expo1COR)/detCOR;
  CoefY1=-(b0COR+b1COR*expo1COR)/detCOR;
  CoefZ2=(p0COR+p1COR*expo2COR+p2COR*expo2COR*expo2COR)/detCOR;
  CoefY2=-(b0COR+b1COR*expo2COR)/detCOR;
  CoefR=p2COR/q2COR;
}

void cvOneDSubdomain::SetBoundRCRValues(double *rcr, int num){//added IV 050803
  
  rcrTime = 0.0;
  
  assert(num==3);
  proximalResistance = rcr[0];
  capacitance = rcr[1] ;
  distalResistance= rcr[2];
  alphaRCR = (proximalResistance+distalResistance)/(proximalResistance*distalResistance*capacitance);
}

void cvOneDSubdomain::SetBoundWaveValues(double *wave, int num){//added IV 080603
  
  waveTime = 0.0;
  numWavePts = num;
  waveSo = wave[0];
  waveLT = wave[1];
  waveEndh = wave [2];
  waveN = mat->GetN(waveSo);//doesn't depend on So in fact
  waveAlpha = waveN/2/waveSo;
  waveSpeed = mat->GetRefWaveSpeed(waveSo); 
  numWaveMod = wave[3];
  eigValWave = new double[int(ceil(numWaveMod))];
  assert(waveLT<2*PI*waveSo*waveSpeed/fabs(waveN));//assert eigenVal>0
  for(int i=0 ; i < numWaveMod ; i++){
    eigValWave[i] = sqrt(pow((i+1)*PI/waveLT,2)-pow(waveAlpha/waveSpeed,2));
  }
}

// Returns interpolated pressure from specific boundary pressure wave
double cvOneDSubdomain::GetPressure(double currentTime){
  if (currentTime == 0){
    double pressure= P_initial;
    return pressure;
  }
  
  if(pressureTime == NULL || pressureWave == NULL){ 
    cout << "ERROR: pressure information is not prescribed"<< endl; 
    exit(1); 
  } 
  
  // flow rate is assumed to be periodic
  double cycleTime = pressureTime[numPressurePts-1];
  double correctedTime = currentTime - static_cast<long>(currentTime / cycleTime) * cycleTime; 

  double pressure=0.;
  if(correctedTime >= 0 && correctedTime <= pressureTime[0]){
    double xi = (correctedTime - pressureTime[numPressurePts-1]) / (pressureTime[0] - pressureTime[numPressurePts-1]); 
    pressure = (pressureWave[numPressurePts-1] + xi * (pressureWave[0] - pressureWave[numPressurePts-1])); 
  }else{
    int ptr = 0; 
    bool wasFound = false; 
    while( !wasFound){ 
      if( correctedTime >= pressureTime[ptr] && correctedTime <= pressureTime[ptr+1]){
        wasFound = true; 
      }else{
        ptr++;   
      }      
    }
 
    // linear interpolation between values 
    double xi = (correctedTime - pressureTime[ptr]) / (pressureTime[ptr+1] - pressureTime[ptr]); 
    pressure = (pressureWave[ptr] + xi * (pressureWave[ptr+1] - pressureWave[ptr])); 
  }
  return pressure;
}

// Returns interpolated resistance from specific boundary resistance wave
double cvOneDSubdomain::GetBoundResistance(double currentTime){

  if(resistanceTime == NULL || resistanceWave == NULL){ 
    cout << "ERROR: resistance information is not prescribed"<< endl; 
    exit(1); 
  } 
  
  // Flow rate is assumed to be periodic
  double cycleTime = resistanceTime[numPressurePts-1];
  double correctedTime = currentTime - static_cast<long>(currentTime / cycleTime) * cycleTime; 

  double resistance=0.;
  if(correctedTime >= 0 && correctedTime <= resistanceTime[0]){
    double xi = (correctedTime - resistanceTime[numPressurePts-1]) / (resistanceTime[0] - resistanceTime[numPressurePts-1]); 
    resistance = (resistanceWave[numPressurePts-1] + xi * (resistanceWave[0] - resistanceWave[numPressurePts-1])); 
  }else{
    int ptr = 0; 
    bool wasFound = false; 
    while( !wasFound){ 
      if( correctedTime >= resistanceTime[ptr] && correctedTime <= resistanceTime[ptr+1]){
        wasFound = true; 
      }else{
        ptr++; 
      }
    }
    // linear interpolation between values 
    double xi = (correctedTime - resistanceTime[ptr]) / (resistanceTime[ptr+1] - resistanceTime[ptr]); 
    resistance = (resistanceWave[ptr] + xi * (resistanceWave[ptr+1] - resistanceWave[ptr])); 
  }
  return resistance;
}

double cvOneDSubdomain::GetBoundAreabyPresWave(double currentTime){ 
  double pressure =  GetPressure(currentTime);
  return mat->GetArea(pressure, GetLength()); 
} 

// Use this for impedance boundary condition in convolution
double* cvOneDSubdomain::ShiftPressure(double lastP, double currentTime,double currS){
  // get the last period of computed pressure.  if less than numImpedancePts, padded with P0=0.0 mmHg.
  // scoot all values one over 
  
  if(currentTime != impedanceTime){
    for(int i=numImpedancePts-1;i>0;i--){
      impedancePressure[i] = impedancePressure[i-1];
    }
    if(cvOneDGlobal::CONSERVATION_FORM==0){//Brooke's formulation
      impedanceTime = currentTime;
    }
    if(cvOneDGlobal::CONSERVATION_FORM==1){//Irene's formulation
      // impedanceTime = currentTime;//I don't want the impedance time to change yet IV
      // cout<<"shiftpressure"<<impedanceTime<<endl;
    }
  }
  impedancePressure[0]=lastP;
  return impedancePressure;
}


/*double* Subdomain::GetDpDs(double lastDpDs)// don't think I use this
{
  impedanceDpDs[0]=lastDpDs;
  return impedanceDpDs;  
}
*/
void cvOneDSubdomain::SaveK(double k, int i){  
  K[i]=k;
}



/*
* Use the following functions MemIntRCR, MemAdvRCR, ConvPressRCR for RCR BC conditions
* Mem is for "memory" - time integrals
* all added IV 050803
*/

double cvOneDSubdomain::MemIntRCR(double currP, double previousP, double deltaTime, double currentTime){  
  double MemIrcr;
  MemIrcr = ConvPressRCR(previousP, deltaTime, currentTime)*expmDtOne(deltaTime)/(alphaRCR*alphaRCR) 
    + currP/alphaRCR*(deltaTime-expmDtOne(deltaTime)/alphaRCR);    
  return MemIrcr;  
}

double cvOneDSubdomain::MemAdvRCR(double currP, double previousP, double deltaTime, double currentTime){  
  double MemK;
  double Coeff = MemC(currP, previousP, deltaTime, currentTime);
  
  MemK = Coeff*Coeff/2/alphaRCR*(1-exp(-2*alphaRCR*deltaTime)) 
    + 2*Coeff*currP/alphaRCR/(proximalResistance + distalResistance)*expmDtOne(deltaTime)
    + deltaTime*pow(currP/(proximalResistance+distalResistance),2);
  return MemK;
}

double cvOneDSubdomain::MemC(double currP, double previousP, double deltaTime, double currentTime){  
  double prevTime = currentTime-deltaTime;
  double Cadv;
  Cadv = - ConvPressRCR(previousP, deltaTime, currentTime)/(alphaRCR*proximalResistance*proximalResistance*capacitance)+
    (Q_initial - mat->GetReferencePressure()/proximalResistance)*exp(-alphaRCR*prevTime) 
    + currP/(alphaRCR*proximalResistance*proximalResistance*capacitance);
  return Cadv;
}

double cvOneDSubdomain::dMemCdP(void){  
  double diffMemCdP;
  diffMemCdP = 1/(alphaRCR*capacitance)/pow(proximalResistance,2);
  return diffMemCdP;
}

double cvOneDSubdomain::ConvPressRCR(double previousP, double deltaTime, double currentTime){  
  // Initialization
  if(currentTime <= deltaTime ){MemD = 0.0;}  
  // Convoluted pressure   
  if(currentTime > deltaTime && currentTime != rcrTime){
    MemD = MemD*exp(-alphaRCR*deltaTime)+previousP*expmDtOne(deltaTime);//all the deltaTime are from previous time step
    rcrTime = currentTime;
  }
  return MemD;
}

double cvOneDSubdomain::dMemIntRCRdP(double deltaTime){  
  double dMemIdP;
  dMemIdP = (deltaTime - expmDtOne(deltaTime)/alphaRCR)/alphaRCR;
  // Cout << dMemIdP ;
  return dMemIdP;
}


double cvOneDSubdomain::dMemAdvRCRdP(double currP, double prevP, double deltaTime, double currentTime){  
  double MemCoeff = MemC(currP, prevP, deltaTime, currentTime);
  double dMemKdP;
  dMemKdP = (1-exp(-2*alphaRCR*deltaTime))/alphaRCR*MemCoeff*dMemCdP() 
    + expmDtOne(deltaTime)/alphaRCR/(proximalResistance + distalResistance)*(MemCoeff + currP*dMemCdP())
    + 2*deltaTime*currP/pow(proximalResistance + distalResistance,2);
  //cout<<" dMemKdP"<<" "<<dMemKdP<<endl;
  return dMemKdP;
}

/* 082605 kimhj
* Adding a new boundary condition for coronary arteries
* Use Mem for "memory" - time integrals 
*/
double cvOneDSubdomain::ConvPressCoronary(double previousP, double deltaTime, double currentTime, double exponent){

  double prevTime=currentTime-deltaTime;
  if (currentTime <= deltaTime) { 
    MemD1=0.0;
    MemD2=0.0;
  }
    
  if (currentTime > deltaTime && currentTime!=corTime) {
    MemD1=MemD1*exp(expo1COR*deltaTime)+expmDtOneCoronary(deltaTime, expo1COR)*(CoefZ1*previousP+CoefY1*getBoundCoronaryValues(prevTime));
    MemD2=MemD2*exp(expo2COR*deltaTime)+expmDtOneCoronary(deltaTime, expo2COR)*(CoefZ2*previousP+CoefY2*getBoundCoronaryValues(prevTime));
    corTime=currentTime;
  }
  if (exponent==expo1COR){ return MemD1; } 
  else if (exponent==expo2COR){ return MemD2; }
  cout << "ConvPressCoronary return value undefined!" << std::endl;
  assert(0);
  return 0.0;
}

double cvOneDSubdomain::expmDtOneCoronary(double deltaTime, double exponent){
  double lambda=exponent;
  double expmDt=(exp(lambda*deltaTime)-1)/lambda;
  return expmDt;
}//integral(exp(lambda*(t-tn)),tn,tn+1)

double cvOneDSubdomain::MemIntCoronary(double currP, double previousP, double deltaTime, double currentTime, double exponent){
  double MemIcor = 0.0;
  double lambda = exponent;
  if (lambda==expo1COR) {
    MemIcor = ConvPressCoronary(previousP, deltaTime, currentTime, lambda)*expmDtOneCoronary(deltaTime,lambda) 
    + (CoefZ1*currP+CoefY1*getBoundCoronaryValues(currentTime))*(-deltaTime+expmDtOneCoronary(deltaTime,lambda))/lambda;
  }
  else if (lambda==expo2COR) {
    MemIcor = ConvPressCoronary(previousP, deltaTime, currentTime, lambda)*expmDtOneCoronary(deltaTime,lambda) 
    + (CoefZ2*currP+CoefY2*getBoundCoronaryValues(currentTime))*(-deltaTime+expmDtOneCoronary(deltaTime,lambda))/lambda;
  }
  return MemIcor;  
}

double cvOneDSubdomain::CORic1(void){
  double CORic1=1/detCOR*(q2COR*(dQ_dT_initial-expo2COR*Q_initial)
       -(p1COR+p2COR*expo1COR)*GetPressure(0)-p2COR*GetdPressuredt(0)
       +b1COR*getBoundCoronaryValues(0));
  return CORic1;
}

double cvOneDSubdomain::CORic2(void){
  double CORic2=1/detCOR*(q2COR*(dQ_dT_initial-expo1COR*Q_initial)
       -(p1COR+p2COR*expo2COR)*GetPressure(0)-p2COR*GetdPressuredt(0)
       +b1COR*getBoundCoronaryValues(0));
  return CORic2;
}

double cvOneDSubdomain::MemCoronary1(double currP, double previousP, double deltaTime, double currentTime){  
  double prevTime = currentTime-deltaTime;
  double Cadv;
  Cadv = ConvPressCoronary(previousP, deltaTime, currentTime, expo1COR)+CORic1()*exp(expo1COR*prevTime) 
    + (currP*CoefZ1+CoefY1*getBoundCoronaryValues(currentTime))/expo1COR;
  return Cadv;
}

double cvOneDSubdomain::MemCoronary2(double currP, double previousP, double deltaTime, double currentTime){  
  double prevTime = currentTime-deltaTime;
  double Cadv;
  Cadv = ConvPressCoronary(previousP, deltaTime, currentTime, expo2COR)+CORic2()*exp(expo2COR*prevTime) 
    + (currP*CoefZ2+CoefY2*getBoundCoronaryValues(currentTime))/expo2COR;
  return Cadv;
}

double cvOneDSubdomain::dMemCoronary1dP(void){  
  double diffMemCdP;
  diffMemCdP = CoefZ1/expo1COR;
  return diffMemCdP;
}

double cvOneDSubdomain::dMemCoronary2dP(void){  
  double diffMemCdP;
  diffMemCdP = CoefZ2/expo2COR;
  return diffMemCdP;
}

double cvOneDSubdomain::dMemIntCoronarydP(double deltaTime, double exponent){  
  double dMemIdP = 0.0;
  double lambda=exponent;
  if (lambda==expo1COR) { dMemIdP = CoefZ1*(-deltaTime+expmDtOneCoronary(deltaTime,lambda))/lambda; }
  else if (lambda==expo2COR) { dMemIdP = CoefZ2*(-deltaTime+expmDtOneCoronary(deltaTime,lambda))/lambda; }
  return dMemIdP;
}

double cvOneDSubdomain::MemAdvCoronary(double currP, double previousP, double deltaTime, double currentTime){  
  double MemK;
  double Coeff1 = MemCoronary1(currP, previousP, deltaTime, currentTime);
  double Coeff2 = MemCoronary2(currP, previousP, deltaTime, currentTime);
  double currPlv = getBoundCoronaryValues(currentTime);
  double factor = (p0COR*currP-b0COR*currPlv)/q0COR;
  MemK = Coeff1*Coeff1/2/expo1COR*(exp(2*expo1COR*deltaTime)-1) 
    - 2*Coeff1*Coeff2*expmDtOneCoronary(deltaTime, expo1COR+expo2COR)
    + Coeff2*Coeff2/2/expo2COR*(exp(2*expo2COR*deltaTime)-1)
    +pow(factor,2)*deltaTime
    +2*Coeff1*factor*expmDtOneCoronary(deltaTime, expo1COR)
    -2*Coeff2*factor*expmDtOneCoronary(deltaTime, expo2COR);
  return MemK;
}

double cvOneDSubdomain::dMemAdvCoronarydP(double currP, double prevP, double deltaTime, double currentTime){  
  double Coeff1 = MemCoronary1(currP, prevP, deltaTime, currentTime);
  double Coeff2 = MemCoronary2(currP, prevP, deltaTime, currentTime);
  double currPlv = getBoundCoronaryValues(currentTime);
  double factor = (p0COR*currP-b0COR*currPlv)/q0COR;
  double dMemKdP;
  dMemKdP = (Coeff1/expo1COR*(exp(2*expo1COR*deltaTime)-1)
    -2*Coeff2*expmDtOneCoronary(deltaTime, expo1COR+expo2COR)
    +2*factor*expmDtOneCoronary(deltaTime, expo1COR))*dMemCoronary1dP()
    +(-2*Coeff1*expmDtOneCoronary(deltaTime, expo1COR+expo2COR)
    +Coeff2/expo2COR*(exp(2*expo2COR*deltaTime)-1)
    -2*factor*expmDtOneCoronary(deltaTime, expo2COR))*dMemCoronary2dP()
    +(2*factor*deltaTime+2*Coeff1*expmDtOneCoronary(deltaTime, expo1COR)
    -2*Coeff2*expmDtOneCoronary(deltaTime, expo2COR))*p0COR/q0COR;
  return dMemKdP;
}

/*
* Use the following functions MemIntWave, MemAdvWave, ConvPressWave for Wave BC conditions
* Mem is for "memory" - time integrals
* all added IV 080603
*/
double cvOneDSubdomain::memIniWave(double initS,double Time){   
  WaveIni = 0.0;
  for(int j=0;j<numWaveMod;j++){
    WaveIni += sin(waveSpeed*eigValWave[j]*Time)/eigValWave[j];
    // cout<<sin(waveSpeed*eigValWave[j]*Time)/eigValWave[j]*(-2.0)*waveSpeed*initS*exp(waveAlpha*Time)/waveLT<<endl;
  }
  WaveIni *= -2.0*waveSpeed*initS*exp(waveAlpha*Time)/waveLT;
  // cout<<"waveIni"<<WaveIni<<endl;
  return WaveIni;
  //return 0.0;//gr1 only
}

double cvOneDSubdomain::memEndBCWave(double Time){   
  WaveEndBC = waveSpeed*waveSpeed*waveSo*waveEndh*(1.0-exp(waveN/waveSo*Time))/waveN/waveLT; 
  return WaveEndBC;
}

double cvOneDSubdomain::convolWave(double currS, double prevS, double deltaTime, double currentTime, double Time){     
  double prevTime = currentTime-deltaTime;
  //initialization
  if(currentTime == deltaTime && currentTime != waveTime){
    MemDWave = 0.0;
  }
  if(currentTime > deltaTime && currentTime != waveTime){
    MemDWave = MemDWave + prevS*exp(-prevTime*waveN/waveSo)*(exp(waveN/waveSo*deltaTime)-1.0);    
  }   
  MemEWave = (1.0+2.0*numWaveMod)*waveSpeed*waveSpeed/waveLT*waveSo/waveN*exp(waveN/waveSo*Time)*(MemDWave+currS*(exp(-prevTime*waveN/waveSo)-exp(-Time*waveN/waveSo)));
  // MemEWave = MemEWave/(1.0+2.0*numWaveMod);//check gr1
  return MemEWave;
}

double cvOneDSubdomain::dblConvolWave(double currS, double prevS, double deltaTime, double currentTime, double Time){   
  double prevTime = currentTime-deltaTime;
  complex<double> COMPLX(0.0,1.0);
  //initialization
  if(currentTime == deltaTime && currentTime != waveTime){
    MemEIw = new complex<double>[int(ceil(numWaveMod))];
    MemEIw1 = new complex<double>[int(ceil(numWaveMod))];
    MemEIw2 = new complex<double>[int(ceil(numWaveMod))];
    MemEISpWave = new complex<double>[int(ceil(numWaveMod))];
    MemEISmWave = new complex<double>[int(ceil(numWaveMod))];
    MemEIS2pWave = new complex<double>[int(ceil(numWaveMod))];
    MemEIS2mWave = new complex<double>[int(ceil(numWaveMod))];
  }
  MemEIWaveM = 0.0;
  for(int j=0;j<numWaveMod;j++){

    double parenth = 1+exp(waveN/waveSo*deltaTime)-2*exp(waveAlpha*deltaTime)*cos(waveSpeed*eigValWave[j]*deltaTime);

    if(currentTime == deltaTime && currentTime != waveTime){
      MemEISpWave[j] = 0.0;
      MemEISmWave[j] = 0.0;
      MemEIS2pWave[j] = 0.0;
      MemEIS2mWave[j] = 0.0;
      MemEIw1[j]=0.0;
    }
    if(currentTime == 2*deltaTime && currentTime != waveTime){
      MemEISpWave[j] = 0.0;
      MemEISmWave[j] = 0.0;
      MemEIS2pWave[j] += prevS*exp(-waveAlpha*prevTime)*exp(COMPLX*waveSpeed*eigValWave[j]*prevTime);
      MemEIS2mWave[j] += prevS*exp(-waveAlpha*prevTime)*exp(-COMPLX*waveSpeed*eigValWave[j]*prevTime);
      MemEIw1[j] += MemEISpWave[j] + MemEISmWave[j]-prevS*exp(-waveN/waveSo*prevTime)*(waveSo/waveN*waveSpeed*eigValWave[j]*(1-exp(waveN/waveSo*deltaTime))+exp(waveAlpha*deltaTime)*sin(waveSpeed*eigValWave[j]*deltaTime));
    }
    if(currentTime > 2*deltaTime && currentTime != waveTime){
      MemEISpWave[j] = MemEISpWave[j]*exp(-waveAlpha*deltaTime)*exp(COMPLX*waveSpeed*eigValWave[j]*deltaTime) + previous2S*exp(waveAlpha*(-2.0*prevTime+deltaTime))*exp(COMPLX*waveSpeed*eigValWave[j]*deltaTime)/2.0/COMPLX*parenth;
      MemEISmWave[j] = MemEISmWave[j]*exp(-waveAlpha*deltaTime)*exp(-COMPLX*waveSpeed*eigValWave[j]*deltaTime) - previous2S*exp(waveAlpha*(-2.0*prevTime+deltaTime))*exp(-COMPLX*waveSpeed*eigValWave[j]*deltaTime)/2.0/COMPLX*parenth;
      MemEIS2pWave[j] += prevS*exp(-waveAlpha*prevTime)*exp(COMPLX*waveSpeed*eigValWave[j]*prevTime);
      MemEIS2mWave[j] += prevS*exp(-waveAlpha*prevTime)*exp(-COMPLX*waveSpeed*eigValWave[j]*prevTime);
      MemEIw1[j] += MemEISpWave[j] + MemEISmWave[j]-prevS*exp(-waveN/waveSo*prevTime)*(waveSo/waveN*waveSpeed*eigValWave[j]*(1-exp(waveN/waveSo*deltaTime))+exp(waveAlpha*deltaTime)*sin(waveSpeed*eigValWave[j]*deltaTime));
      // cout<<-prevS*exp(-waveN/waveSo*prevTime)*(waveSo/waveN*waveSpeed*eigValWave[j]*(1-exp(waveN/waveSo*deltaTime))+exp(waveAlpha*deltaTime)*sin(waveSpeed*eigValWave[j]*deltaTime))<<endl;
      // cout<<"MemEIw1  "<<-deltaTime*(MemEIw1[j]*2.0*waveSpeed/waveLT/eigValWave[j]*exp(waveN/waveSo*Time)).real();
    }
//*
    MemEIw2[j] = MemEIS2pWave[j]/2.0/COMPLX*(exp(-COMPLX*waveSpeed*eigValWave[j]*Time)*exp(-waveAlpha*Time)-exp(-COMPLX*waveSpeed*eigValWave[j]*prevTime)*exp(-waveAlpha*prevTime)-exp(waveAlpha*deltaTime)*(exp(-waveAlpha*Time)*exp(-COMPLX*waveSpeed*eigValWave[j]*(Time+deltaTime))-exp(-waveAlpha*prevTime)*exp(-COMPLX*waveSpeed*eigValWave[j]*(prevTime+deltaTime)))) 
                 - MemEIS2mWave[j]/2.0/COMPLX*(exp(COMPLX*waveSpeed*eigValWave[j]*Time)*exp(-waveAlpha*Time)-exp(COMPLX*waveSpeed*eigValWave[j]*prevTime)*exp(-waveAlpha*prevTime)-exp(waveAlpha*deltaTime)*(exp(-waveAlpha*Time)*exp(COMPLX*waveSpeed*eigValWave[j]*(Time+deltaTime))-exp(-waveAlpha*prevTime)*exp(COMPLX*waveSpeed*eigValWave[j]*(prevTime+deltaTime))))
                 + currS*(sin(waveSpeed*eigValWave[j]*(prevTime-Time))*exp(-waveAlpha*(Time+prevTime))+waveSo/waveN*waveSpeed*eigValWave[j]*(exp(-waveN/waveSo*prevTime)-exp(-waveN/waveSo*Time)));
//*/   
  /*
      cout<<"   + "<< (-deltaTime*2.0*waveSpeed/waveLT/eigValWave[j]*exp(waveN/waveSo*Time)*MemEIS2pWave[j]/2.0/COMPLX*(exp(-COMPLX*waveSpeed*eigValWave[j]*Time)*exp(-waveAlpha*Time)-exp(-COMPLX*waveSpeed*eigValWave[j]*prevTime)*exp(-waveAlpha*prevTime)-exp(waveAlpha*deltaTime)*(exp(-waveAlpha*Time)*exp(-COMPLX*waveSpeed*eigValWave[j]*(Time+deltaTime))-exp(-waveAlpha*prevTime)*exp(-COMPLX*waveSpeed*eigValWave[j]*(Time+deltaTime))))).real() 
          <<"   -  "<<(deltaTime*2.0*waveSpeed/waveLT/eigValWave[j]*exp(waveN/waveSo*Time)*MemEIS2mWave[j]/2.0/COMPLX*(exp(COMPLX*waveSpeed*eigValWave[j]*Time)*exp(-waveAlpha*Time)-exp(COMPLX*waveSpeed*eigValWave[j]*prevTime)*exp(-waveAlpha*prevTime)-exp(waveAlpha*deltaTime)*(exp(-waveAlpha*Time)*exp(COMPLX*waveSpeed*eigValWave[j]*(Time+deltaTime))-exp(-waveAlpha*prevTime)*exp(COMPLX*waveSpeed*eigValWave[j]*(Time+deltaTime))))).real()
          <<"   currS  "<<-deltaTime*2.0*waveSpeed/waveLT/eigValWave[j]*exp(waveN/waveSo*Time)*currS*(sin(waveSpeed*eigValWave[j]*(prevTime-Time))*exp(-waveAlpha*(Time+prevTime))+waveSo/waveN*waveSpeed*eigValWave[j]*(exp(-waveN/waveSo*prevTime)-exp(-waveN/waveSo*Time)))<<"   "
          <<endl;
  //*/    //cout<<"   MemEIw2  "<<-deltaTime*(MemEIw2[j]*2.0*waveSpeed/waveLT/eigValWave[j]*exp(waveN/waveSo*Time)).real()<<endl;

      //MemEIw2[j]=0.0;  
    MemEIw[j] = MemEIw1[j]+MemEIw2[j];
    MemEIWaveM -= MemEIw[j]*2.0*waveSpeed/waveLT/eigValWave[j]*exp(waveN/waveSo*Time);
  }

  previous2S = prevS;
  MemEIWave = MemEIWaveM.real();
  //cout<<"MemEIWave    "<<MemEIWave*deltaTime<<endl;
  return MemEIWave;
  //return 0.0;//gr1 only
}

double cvOneDSubdomain::QWave(double currS, double prevS, double initS, double deltaTime, double currentTime, double Time){   
  double Qwave = memEndBCWave(Time) + convolWave(currS, prevS, deltaTime, currentTime,Time) + memIniWave(initS,Time) + dblConvolWave(currS,prevS,deltaTime,currentTime,Time);
  if(currentTime != waveTime){waveTime = currentTime;}//to check
  return Qwave;
}

double cvOneDSubdomain::MemQWave(double currS, double prevS, double initS, double deltaTime,double currentTime){   
  double MemInt;
  // MemInt = (QWave(currS,prevS,initS,deltaTime,currentTime,currentTime) + QWave(currS,prevS,initS,deltaTime,currentTime,currentTime-deltaTime))*deltaTime/2;//integration by trapezoidal rule
  // MemInt = (QWave(currS,prevS,initS,deltaTime,currentTime,currentTime)+4*QWave(currS,prevS,initS,deltaTime,currentTime,currentTime-deltaTime/2)  + QWave(currS,prevS,initS,deltaTime,currentTime,currentTime-deltaTime))*deltaTime/6;//integration by simpson rule
  // MemInt = QWave(currS,prevS,initS,deltaTime,currentTime,currentTime)*deltaTime;//integration with SpaceTime 
  MemInt = QWave(currS,prevS,initS,deltaTime,currentTime,currentTime-deltaTime)*deltaTime;//integration with SpaceTime tn
  return MemInt;
}

double cvOneDSubdomain::MemAdvWave(double currS, double prevS,double initS,double deltaTime,double currentTime){  
  double MemK;
  // double QMemKEnd = QWave(currS,prevS,initS,deltaTime,currentTime,currentTime);
  double QMemKBeg = QWave(currS,prevS,initS,deltaTime,currentTime,currentTime-deltaTime);
  // double QMemKMid = QWave(currS,prevS,initS,deltaTime,currentTime,currentTime-deltaTime/2);
  // MemK = (QMemKBeg*QMemKBeg + QMemKEnd*QMemKEnd)/2*deltaTime;//trapeziodal rule
  // MemK = (QMemKBeg*QMemKBeg + 4*QMemKMid*QMemKMid+ QMemKEnd*QMemKEnd)/6*deltaTime;//simpson rule
  // MemK = QMemKEnd*QMemKEnd*deltaTime;//Space Time
  MemK = QMemKBeg*QMemKBeg*deltaTime;//Space Time
  return MemK;
}

double cvOneDSubdomain::dMemQWavedS(double deltaTime, double currentTime){  
  double dMemIdS;
  double prevTime = currentTime - deltaTime;
  double dQWavedS = (1.0+2.0*numWaveMod)*waveSpeed*waveSpeed/waveLT*waveSo/waveN*exp(waveN/waveSo*currentTime)*(exp(-prevTime*waveN/waveSo)-exp(-currentTime*waveN/waveSo));//contribution from ConvolWave
  ///*
  for(int j=0; j <numWaveMod;j++){
    dQWavedS -= 2.0*waveSpeed/waveLT/eigValWave[j]*exp(waveN/waveSo*currentTime)*(sin(waveSpeed*eigValWave[j]*(prevTime-currentTime))*exp(-waveAlpha*(currentTime+prevTime))+waveSo/waveN*waveSpeed*eigValWave[j]*(exp(-waveN/waveSo*prevTime)-exp(-waveN/waveSo*currentTime)));//contribution from dblConvolWave
  }//*/
  /*double dQWaveEnddS= (1.0+2.0*numWaveMod)*waveSpeed*waveSpeed/waveLT*waveSo/waveN*exp(waveN/waveSo*currentTime)*(exp(-prevTime*waveN/waveSo)-exp(-currentTime*waveN/waveSo));//contribution from ConvolWave
  double dQWaveMiddS= (1.0+2.0*numWaveMod)*waveSpeed*waveSpeed/waveLT*waveSo/waveN*exp(waveN/waveSo*(currentTime-deltaTime/2))*(exp(-prevTime*waveN/waveSo)-exp(-(currentTime-deltaTime/2)*waveN/waveSo));//contribution from ConvolWave
  for(int j=0; j <numWaveMod;j++){
    dQWaveEnddS -= 2.0*waveSpeed/waveLT/eigValWave[j]*exp(waveN/waveSo*currentTime)*(sin(waveSpeed*eigValWave[j]*(prevTime-currentTime))*exp(-waveAlpha*(currentTime+prevTime))+waveSo/waveN*waveSpeed*eigValWave[j]*(exp(-waveN/waveSo*prevTime)-exp(-waveN/waveSo*currentTime)));
    dQWaveMiddS -= 4*2.0*waveSpeed/waveLT/eigValWave[j]*exp(waveN/waveSo*(currentTime-deltaTime/2))*(sin(waveSpeed*eigValWave[j]*(-deltaTime/2))*exp(-waveAlpha*((currentTime-deltaTime/2)+prevTime))+waveSo/waveN*waveSpeed*eigValWave[j]*(exp(-waveN/waveSo*prevTime)-exp(-waveN/waveSo*(currentTime-deltaTime/2))));
  }
  dMemIdS = (dQWaveEnddS+dQWaveMiddS)*deltaTime/6;//simpson rule //*/
  //dMemIdS = dQWavedS*deltaTime/2;//trapzoidal rule
  dMemIdS = dQWavedS*deltaTime;//Space Time
  //dMemIdS = dMemIdS/(1.0 + 2*numWaveMod);//gr1 check
  //return dMemIdS;
  return 0.0;
}

double cvOneDSubdomain::dMemAdvWavedS(double currS,double prevS,double initS,double deltaTime, double currentTime){  
  double dMemKdS;
  double QdMemKdS = QWave(currS,prevS,initS,deltaTime,currentTime,currentTime);
  dMemKdS = 2*dMemQWavedS(deltaTime,currentTime)*QdMemKdS;//trapzoidal rule and space time

  /*double QdMemKdSMid = QWave(currS,prevS,initS,deltaTime,currentTime,currentTime-deltaTime/2);
  double prevTime=currentTime-deltaTime;
  double dQWaveEnddS= (1.0+2.0*numWaveMod)*waveSpeed*waveSpeed/waveLT*waveSo/waveN*exp(waveN/waveSo*currentTime)*(exp(-prevTime*waveN/waveSo)-exp(-currentTime*waveN/waveSo));//contribution from ConvolWave
  double dQWaveMiddS= (1.0+2.0*numWaveMod)*waveSpeed*waveSpeed/waveLT*waveSo/waveN*exp(waveN/waveSo*(currentTime-deltaTime/2))*(exp(-prevTime*waveN/waveSo)-exp(-(currentTime-deltaTime/2)*waveN/waveSo));//contribution from ConvolWave
 for(int j=0; j <numWaveMod;j++){
      dQWaveEnddS -= 2.0*waveSpeed/waveLT/eigValWave[j]*exp(waveN/waveSo*currentTime)*(sin(waveSpeed*eigValWave[j]*(prevTime-currentTime))*exp(-waveAlpha*(currentTime+prevTime))+waveSo/waveN*waveSpeed*eigValWave[j]*(exp(-waveN/waveSo*prevTime)-exp(-waveN/waveSo*currentTime)));
      dQWaveMiddS -= 4*2.0*waveSpeed/waveLT/eigValWave[j]*exp(waveN/waveSo*(currentTime-deltaTime/2))*(sin(waveSpeed*eigValWave[j]*(-deltaTime/2))*exp(-waveAlpha*((currentTime-deltaTime/2)+prevTime))+waveSo/waveN*waveSpeed*eigValWave[j]*(exp(-waveN/waveSo*prevTime)-exp(-waveN/waveSo*(currentTime-deltaTime/2))));

  }
  dMemKdS=deltaTime/6*(2*dQWaveEnddS*QdMemKdS+2*dQWaveMiddS*QdMemKdSMid);//simpson rule //*/
  
  //return 0.0;
  return dMemKdS;
}

/*
* Use the following functions MemIntImp, MemAdvImp, ConvPressImp for impedance BC conditions
* Mem is for "memory" - time integrals
* all added IV 050803
*/
double cvOneDSubdomain::MemIntImp(double *press, double deltaTime, double currentTime){  
  double MemIimp;
  MemIimp = deltaTime*ConvPressImp(press, currentTime);
  return MemIimp;
}

double cvOneDSubdomain::MemAdvImp(double *press, double deltaTime, double currentTime){  
  double MemK;
  MemK = deltaTime*pow(ConvPressImp(press, currentTime),2);
  return MemK;
}

double cvOneDSubdomain::dMemIntImpdP(double deltaTime){  
  double dMemIdP;
  //dMemIdP = deltaTime* impedance[0];
  dMemIdP = deltaTime* impedance[0]/numImpedancePts;//vie 112904 consistent impedance 
  //cout << dMemIdP ;
  return dMemIdP;
}

double cvOneDSubdomain::dMemAdvImpdP(double *press, double deltaTime, double currentTime){  
  double dMemKdP;
  //dMemKdP = deltaTime*2*impedance[0]*ConvPressImp(press, currentTime);
  dMemKdP = deltaTime*2*impedance[0]/numImpedancePts*ConvPressImp(press, currentTime);//vie 112904 consistent impedance 
  //cout<<" dMemKdP"<<" "<<dMemKdP<<endl;
  return dMemKdP;
}

double cvOneDSubdomain::ConvPressImp(double *press, double currentTime){  
  double MemDImpc = 0.0; 
  // convoluted pressure 
  //first compute the convolution of the known values-memory terms at the beginning of each new time step
  if(currentTime != impedanceTime){
    MemDImp= 0.0;
    for(int j=1;j<numImpedancePts;j++){
      //MemDImp += impedance[j]*press[j];  
      MemDImp += impedance[j]/numImpedancePts*press[j]; //vie 112904 consistent impedance 
      }
    impedanceTime = currentTime;
  //  cout<<"convpressimp"<<impedanceTime<<endl;
  }
  //add the contribution of the currentSol, which is updated at each iteration
  //MemDImpc = MemDImp + impedance[0]*press[0];
  MemDImpc = MemDImp + impedance[0]/numImpedancePts*press[0];//vie 112904 consistent impedance 
  return MemDImpc;
}