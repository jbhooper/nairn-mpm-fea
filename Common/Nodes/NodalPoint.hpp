/********************************************************************************
    NodalPoint.hpp
    nairn-mpm-fea
    
    Created by John Nairn on Wed Jan 24 2002.
    Copyright (c) 2001 John A. Nairn, All rights reserved.
	
	Dependencies
		none
********************************************************************************/

#ifndef _NODALPOINT_

#define _NODALPOINT_

#ifdef MPM_CODE

class CrackSegment;
class CrackNode;
class TransportTask;
class MaterialContactNode;
#include "Nodes/CrackVelocityField.hpp"
#include "Cracks/CrackHeader.hpp"

#endif

class NodalPoint : public LinkedObject
{
    public:
		// variables
#ifdef MPM_CODE
		CrackVelocityField **cvf;	// crack velocity fields
		TransportField gCond;		// conduction
		//double gTemperature,gMpCp,fcond;		// conduction
		TransportField gDiff;		// diffusion
		//double gConcentration,gVolume,fdiff;	// diffusion
		unsigned short fixedDirection;
		MaterialContactNode *contactData;
	
		static double interfaceEnergy;		// total tracked each time step
		static double frictionWork;			// cumulative friction work
#else
		double gTemperature;		// delta T in FEA
#endif
	
		// constants (not changed in MPM time step)
        double x,y,z;
        int	num;

#ifdef FEA_CODE
        ForceField *fs;
#endif
       
        // constructors and destructors
		NodalPoint(int,double,double);
		NodalPoint(int,double,double,double);
		void CreateNodalPoint(int,double,double,double);
#ifdef MPM_CODE
		NodalPoint(NodalPoint *);
#endif
		virtual ~NodalPoint();
        
        // methods - general and abstract
        virtual void PrintNodalPoint(ostream &);

#ifdef MPM_CODE
        // methods - MPM only
		void InitializeForTimeStep();
		void CopyFieldInitialization(NodalPoint *);
		void UseTheseFields(CrackVelocityField **);
		short AddCrackVelocityField(int,CrackField *);
		void AddMatVelocityField(short,int);
        bool NeedsMatVelocityField(short,int) const;
	
		void AddMassMomentum(MPMBase *,short,int,double,double,double,double,int,bool);
        void AddMassMomentumLast(MPMBase *,short,int,double,double,double,double);
		void AddMomentumTask1(short,int,double,Vector *,int);
		void AddMass(short,int,double);
		void AddMassTask1(short,int,double,int);
		void CopyVolumeGradient(short,int,Vector *);
		void CopyMassAndMomentum(NodalPoint *);
        void CopyMassAndMomentumLast(NodalPoint *);
        void RezeroNodeTask6(double);
        void AddMomentumTask6(short,int,double,Vector *);
	
		void AddFtotTask3(short,int,Vector *);
		void AddFtotSpreadTask3(short,Vector);
		void AddTractionTask3(MPMBase *,short,int,Vector *);
		void AddGravityAndBodyForceTask3(Vector *);
		void CopyGridForces(NodalPoint *);
		void UpdateMomentaOnNode(double);
		void IncrementDelvaTask5(short,int,double,GridToParticleExtrap *) const;

		bool IncrementDelvSideTask8(short,int,double,Vector *,Vector *,double *,CrackSegment *,double) const;
		bool GetCMVelocityTask8(Vector *,Vector *) const;
		short GetFieldForSurfaceParticle(short,int,CrackSegment *) const;
		void SurfaceCrossesCracks(Vector *,Vector *,CrackField *) const;
		int SurfaceCrossesOneCrack(Vector *,Vector *,int) const;
		int SurfaceCrossesOtherCrack(Vector *,Vector *,int) const;
		void CalcCMVelocityTask8(void);
	
		// specific task methods
		void PrepareForFields(void);
        void ZeroDisp(void);
		int GetFieldForCrack(bool,bool,DispField **,int);
        void ZeroDisp(NodalPoint *);
        void CopyUGradientStressEnergy(NodalPoint *);
        void DeleteDisp(void);
        void DeleteDisp(NodalPoint *);
		int NumberParticles(void);
		bool NeedsParticleListed(short vfld);
		int NumberNonrigidCracks(void);
		bool NodeHasNonrigidParticles(void) const;
		bool NodeHasParticles(void) const;
		double GetNodalMass(bool) const;
		void Describe(bool) const;
		void AddDisplacement(short,int,double,Vector *);
		void AddVolume(short,int,double);
        void AddUGradient(short,double,double,double,double,double,int,double);
		void AddMatWeights(double,double *);
		// GRID_JTERMS
        void AddGridVelocity(short,double,double,double);
        void AddEnergy(short,double,double,double,double);
        void AddStress(short,double,Tensor *);
        Vector GetVelocity(short,int);
		void AddKineticEnergyAndMass(double &,double &);
		void CalcVelocityForStrainUpdate(void);
        short GetCMVelocity(Vector *);
        void CalcStrainField(void);
		void Interpolate(NodalPoint *,NodalPoint *,double,int);
		int HasCrackContact(void);
		void CrackContact(int,double,int);
		void CrackContactThree(int,int,double);
		void MaterialContactOnNode(double,int,MaterialContactNode *);
        void GetMatVolumeGradient(int,Vector *) const;
        void SetMomVel(Vector *,int);
        void AddMomVel(Vector *,double,int);
		void ReflectMomVel(Vector *,NodalPoint *,double,double,int);
        void SetFtotDirection(Vector *,double,Vector *);
        void AddFtotDirection(Vector *,double,double,Vector *);
		void ReflectFtotDirection(Vector *,double,NodalPoint *,double,double,Vector *);
		void SetFixedDirection(int);
		void UnsetFixedDirection(int);
		bool CalcTotalMassAndCount(void);
		void RestoreMomenta(void);
		void AddGetContactForce(bool,Vector *,double,Vector *);
		void AddRigidBCInfo(MPMBase *,double,int,Vector *);
		int ReadAndZeroRigidBCInfo(Vector *,double *,double *);
		void MirrorIgnoredCrackFields(void);
#else
        void InitForceField(void);
        void PrintAvgStress(void);
#endif
	
		// class methods
#ifdef MPM_CODE
		static void PreliminaryCalcs(void);
		static NodalPoint *CreateGhostFromReal(NodalPoint *);
#endif
		static NodalPoint *Create2DNode(int,double,double);
		static NodalPoint *Create3DNode(int,double,double,double);
	
	protected:
#ifdef MPM_CODE
		double nodalMass;				// total mass
#endif
    
    private:
		
#ifdef MPM_CODE
        //methods - MPM only
		void AverageStrain(DispField *,DispField *,DispField *,double);
        void AdjustContact(short,short,Vector *,int,int,double);
#endif

};

// List of nodes stored as nd[1] to nd[nnodes]
extern NodalPoint **nd;
extern int nnodes;
extern int *nda;

#endif
