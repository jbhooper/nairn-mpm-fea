/********************************************************************************
    MaterialController.cpp
    NairnFEA
    
    Created by John Nairn on 6/27/05.
    Copyright (c) 2005 John A. Nairn, All rights reserved.
********************************************************************************/

#include "Read_XML/MaterialController.hpp"

// Material types created here
#include "Materials/IsotropicMat.hpp"
#include "Materials/TransIsotropic.hpp"
#include "Materials/Orthotropic.hpp"
#ifdef MPM_CODE
	#include "NairnMPM_Class/NairnMPM.hpp"
	#include "Materials/Viscoelastic.hpp"
	#include "Materials/HillPlastic.hpp"
	#include "Materials/WoodMaterial.hpp"
	#include "Materials/IsoPlasticity.hpp"
	#include "Materials/Mooney.hpp"
    #include "Materials/HEIsotropic.hpp"
	#include "Materials/BistableIsotropic.hpp"
	#include "Materials/RigidMaterial.hpp"
	#include "Materials/CohesiveZone.hpp"
	#include "Materials/PressureLaw.hpp"
    #include "Materials/CoupledSawTooth.hpp"
	#include "Materials/LinearTraction.hpp"
	#include "Materials/CubicTraction.hpp"
	#include "Materials/TrilinearTraction.hpp"
    #include "Materials/IdealGas.hpp"
    #include "Materials/TaitLiquid.hpp"
	#include "Materials/HEMGEOSMaterial.hpp"
	#include "Materials/Neohookean.hpp"
	#include "Materials/ClampedNeohookean.hpp"
	#include "Materials/ContactLaw.hpp"
	#include "Materials/CoulombFriction.hpp"
	#include "Materials/LinearInterface.hpp"
#else
	#include "Materials/ImperfectInterface.hpp"
#endif

#ifdef MPM_CODE
extern CrackHeader *firstCrack;
#endif

MaterialController *matCtrl=NULL;

#pragma mark ParseController: Constructors and Destructor

// throws std::bad_alloc
MaterialController::MaterialController(void) : ParseController()
{
	nameCtrl=new ParseController();
#ifdef MPM_CODE
	autoContactCtrl = new ParseController();
#endif
}

MaterialController::~MaterialController()
{
	// remove saved names
	IsotropicMat *nextMat = (IsotropicMat *)nameCtrl->firstObject;
	while(nextMat!=NULL)
	{	IsotropicMat *prevMat = nextMat;
		nextMat = (IsotropicMat *)prevMat->GetNextObject();
		delete prevMat;
	}
	delete nameCtrl;
	
#ifdef MPM_CODE
	delete autoContactCtrl;
#endif
}

#pragma mark MaterialController: Methods

/* Create new material
	When a new material is added, you must include the
	header above and add a case below to create a new material when the matID
	calls for the new type. All other issues on implementing the material
	should be possible in the new material class alone, without modifying
	any other core code.
	throws std::bad_alloc
*/
int MaterialController::AddMaterial(int matID,char *matName)
{
	MaterialBase *newMaterial;
	
	switch(matID)
	{   case ISOTROPIC:
			newMaterial=new IsotropicMat(matName);
			break;
		case TRANSISO1:
		case TRANSISO2:
			newMaterial=new TransIsotropic(matName,matID);
			break;
		case ORTHO:
			newMaterial=new Orthotropic(matName);
			break;
#ifdef MPM_CODE
		case VISCOELASTIC:
			newMaterial=new Viscoelastic(matName);
			break;
		case MOONEYRIVLIN:
			newMaterial=new Mooney(matName);
			break;
		case HEISOTROPIC:
			newMaterial = new HEIsotropic(matName);
			break;
		case ISOPLASTICITY:
			newMaterial=new IsoPlasticity(matName);
			break;
		case BISTABLEISO:
			newMaterial=new BistableIsotropic(matName);
			break;
		case RIGIDMATERIAL:
			newMaterial=new RigidMaterial(matName);
			break;
		case COHESIVEZONEMATERIAL:
			newMaterial=new CohesiveZone(matName);
			break;
		case PRESSURELAWMATERIAL:
			newMaterial=new PressureLaw(matName);
			break;
		case COUPLEDSAWTOOTHMATERIAL:
			newMaterial=new CoupledSawTooth(matName);
			break;
		case LINEARTRACTIONMATERIAL:
			newMaterial=new LinearTraction(matName);
			break;
		case CUBICTRACTIONMATERIAL:
			newMaterial=new CubicTraction(matName);
			break;
		case HILLPLASTIC:
			newMaterial=new HillPlastic(matName);
			break;
		case WOODMATERIAL:
			newMaterial=new WoodMaterial(matName);
			break;
		case TRILINEARTRACTIONMATERIAL:
			newMaterial=new TrilinearTraction(matName);
			break;
		case IDEALGASMATERIAL:
			newMaterial=new IdealGas(matName);
			break;
		case TAITLIQUID:
			newMaterial=new TaitLiquid(matName);
			break;
		case MGEOSMATERIAL:
		case HEMGEOSMATERIAL:
			newMaterial=new HEMGEOSMaterial(matName);
			break;
        case NEOHOOKEAN:
			newMaterial=new Neohookean(matName);
			break;
		case CLAMPEDNEOHOOKEAN:
			newMaterial=new ClampedNeohookean(matName);
			break;
		case CONTACTLAW:
			newMaterial=new ContactLaw(matName);
			break;
		case COULOMBFRICTIONLAW:
			newMaterial=new CoulombFriction(matName);
			break;
		case LINEARINTERFACELAW:
			newMaterial=new LinearInterface(matName);
			break;
#else
		case INTERFACEPARAMS:
			newMaterial=new ImperfectInterface(matName);
			break;
#endif
		default:
			return FALSE;
	}
	AddObject(newMaterial);
	return TRUE;
}

// assemble into array used in the code
// throws std::bad_alloc
const char *MaterialController::SetMaterialArray(void)
{
	if(numObjects==0)
		return "No materials were defined in the input file or a memory error.";

	int numAlloc = numObjects;
#ifdef MPM_CODE
	numAlloc += autoContactCtrl->numObjects + 1;		// last for fritionless law on crack with traction laws
	bool hasTractionLaws = false;
#endif
	theMaterials = new (nothrow) MaterialBase *[numAlloc];
	if(theMaterials==NULL) return "Memory error creating array of material types.";
	
	// fill with NULL
	int i;
	for(i=0;i<numAlloc;i++) theMaterials[i] = NULL;
	
	// first one (and it is not NULL)
	MaterialBase *obj=(MaterialBase *)firstObject;

	// filled with named materials first
	if(nameCtrl->firstObject!=NULL)
	{	while(obj!=NULL)
		{	int matID = GetIDFromName(obj->name);
			if(matID>0)
			{	// found name, but is it defined more than once or were too many, return error message
				if(matID>numObjects)
					return "More named materials referenced then were defined in the input file.";
				else if(theMaterials[matID-1] != NULL)
				{	char *errMsg=new char[strlen(obj->name)+50];
					strcpy(errMsg,"Duplicate referenced material name: ");
					strcat(errMsg,obj->name);
					return errMsg;
				}
				theMaterials[matID-1]=obj;
#ifdef MPM_CODE
				if(obj->MaterialStyle()==TRACTION_MAT) hasTractionLaws = true;
#endif
			}
			
			// check next material
			obj=(MaterialBase *)obj->GetNextObject(); 
		}
	}
	
	// fill the array with remaining unreferenced materials
	nmat = 0;
	int numUnreferenced = 0;
	obj=(MaterialBase *)firstObject;
	while(obj!=NULL)
	{	int matID = GetIDFromName(obj->name);
		if(matID == 0)
		{	while(theMaterials[nmat] != NULL)
			{	nmat++;
				// following check should not be possible
				if(nmat>=numObjects)
					return "Not enough room for unreferenced materials";
			}
			theMaterials[nmat] = obj;
			numUnreferenced++;
			nmat++;
#ifdef MPM_CODE
			if(obj->MaterialStyle()==TRACTION_MAT) hasTractionLaws = true;
#endif
		}
		obj=(MaterialBase *)obj->GetNextObject(); 
	}
	
	// final number, but must matche sum of referenced and unreferenced
	nmat = numObjects;
	if(nmat != nameCtrl->numObjects+numUnreferenced)
		return "One or more materials was referenced but never defined.";

#ifdef MPM_CODE
	// cached contact laws created from old style input
	ContactLaw *nextLaw = (ContactLaw *)autoContactCtrl->firstObject;
	while(nextLaw!=NULL)
	{	theMaterials[nmat] = nextLaw;
		nmat++;
		nextLaw = (ContactLaw *)nextLaw->GetNextObject();
	}
	
	//  check for cracks with tractions
	if(hasTractionLaws && firstCrack!=NULL)
	{	char tempName[80];
		strcpy(tempName,"Frictioness for Traction-Law Cracks (Auto)");
		theMaterials[nmat] = new CoulombFriction(tempName);
		nmat++;
	}
#endif
	
	return NULL;
}

// pointer to read a material property
char *MaterialController::InputPointer(char *property,int &input,double &gScaling)
{   return ((MaterialBase *)lastObject)->InputMaterialProperty(property,input,gScaling);
}

// materal name
const char *MaterialController::MaterialType(void)
{   return ((MaterialBase *)lastObject)->MaterialType();
}

// set material color (optional)
void MaterialController::SetMatColor(float red,float green,float blue,float alpha)
{	((MaterialBase *)lastObject)->red=red;
	((MaterialBase *)lastObject)->green=green;
	((MaterialBase *)lastObject)->blue=blue;
	((MaterialBase *)lastObject)->alpha=alpha;
}

#ifdef MPM_CODE
// return current material
void MaterialController::SetCriterion(int matCriterion,int setIndex)
{	((MaterialBase *)lastObject)->criterion[setIndex]=matCriterion;
}

// set propagation direction
void MaterialController::SetDirection(int matDirection,int setIndex)
{	((MaterialBase *)lastObject)->matPropagateDirection[setIndex]=matDirection;
}

// set propagation traction material
void MaterialController::SetTractionMat(int mat,int setIndex)
{	((MaterialBase *)lastObject)->tractionMat[setIndex]=mat;
}

// when done with Friction command, create new material friction object
void MaterialController::SetMaterialFriction(int lawID,int otherMatID)
{	((MaterialBase *)lastObject)->SetFriction(lawID,otherMatID);
}

// Called when material that had PDamping command is done
void MaterialController::SetMaterialDamping(void)
{	((MaterialBase *)lastObject)->SetDamping(matPdamping,matFractionPIC);
}

// Implement fraction PIC by setting damping values
// It is initialized to -1 on PDamping command, but always between 0 and 1 if has PIC attribute
// also set matPdamping to large negative number
void MaterialController::SetFractionPIC(void)
{	matFractionPIC = -1.;
	matPdamping = -1.1e12;
}
void MaterialController::SetFractionPIC(double fract)
{	matFractionPIC = fract;
	if(fract<0.)
		matFractionPIC = 0.;
	else if (fract>1.)
		matFractionPIC = 1.;
}

// cache Contact law from old style input to be added later at end of material array
int MaterialController::AddAutoContactLaw(ContactLaw *autoLaw)
{	autoContactCtrl->AddObject(autoLaw);
	return autoContactCtrl->numObjects;
}

// number of contact laws cached
int MaterialController::NumAutoContactLaws(void) { return autoContactCtrl->numObjects; }


#endif

// get MatID for given name, but if name not defined, add a new one to the list
// throws std::bad_alloc
int MaterialController::GetIDFromNewName(char *matname)
{
	// get name and exit if done
	int matID = GetIDFromName(matname);
	if(matID > 0) return matID;
	
	// create new material name
	IsotropicMat *namedMaterial = new IsotropicMat(matname);
	nameCtrl->AddObject(namedMaterial);
	return nameCtrl->numObjects;
}

// get MatID for given name, but if name not defined return 0
int MaterialController::GetIDFromName(char *matname)
{
	int matID = 0, currentMat = 1;
	
	// checked saved names
	IsotropicMat *nextMat = (IsotropicMat *)nameCtrl->firstObject;
	while(nextMat!=NULL)
	{	if(strcmp(matname,nextMat->name)==0)
		{	matID = currentMat;
			break;
		}
		currentMat++;
		nextMat = (IsotropicMat *)nextMat->GetNextObject();
	}
	return matID;
}



