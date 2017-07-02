// This file is part of Tread Marks
// 
// Tread Marks is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// Tread Marks is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with Tread Marks.  If not, see <http://www.gnu.org/licenses/>.

//Class to handle race car physics.

//#include "Poly.h"
#include "Terrain.h"
#include "Vector.h"

#define MAXWHEELPOINTS 10

#define VEHICLE_CAR 0
#define VEHICLE_TANK 1

#define FLAG_NONE 0
#define FLAG_CONTROL 1
#define FLAG_POS 2
#define FLAG_ROT 4
#define FLAG_VEL 8
#define FLAG_POSROTVEL 14
#define FLAG_AUTHORITATIVE 16

struct VehicleStateLump{
	Mat3 RotM, RotVelM;
	Vec3 Pos, Vel;
	float LAccel, RAccel;
//	unsigned int32_t TimeStamp;
	int32_t Flags;
	int32_t InGroundBits;
	float TurRot, TurRotVel;
	VehicleStateLump(){
		ClearVec3(Pos);
		ClearVec3(Vel);
		for(int32_t i = 0; i < 3; i++){
			ClearVec3(RotM[i]);
			ClearVec3(RotVelM[i]);
		}
		LAccel = RAccel = 0.0f;
//		TimeStamp = 0;
		Flags = 0;
		InGroundBits = 0;
		TurRot = TurRotVel = 0.0f;
	};
	void Set(Vec3 pos, Vec3 vel, Mat3 rot, Mat3 rotvel, float trot){//, float tsteer){
		if(pos) CopyVec3(pos, Pos);
		if(vel) CopyVec3(vel, Vel);
		if(rot) for(int32_t i = 0; i < 3; i++) CopyVec3(rot[i], RotM[i]);
		if(rotvel) for(int32_t i = 0; i < 3; i++) CopyVec3(rotvel[i], RotVelM[i]);
	//	TurSteer = tsteer;
		TurRot = trot;
	};
	void Get(Vec3 pos, Vec3 vel, Mat3 rot, Mat3 rotvel, float *trot){//, float &tsteer){
		if(pos) CopyVec3(Pos, pos);
		if(vel) CopyVec3(Vel, vel);
		if(rot) for(int32_t i = 0; i < 3; i++) CopyVec3(RotM[i], rot[i]);
		if(rotvel) for(int32_t i = 0; i < 3; i++) CopyVec3(RotVelM[i], rotvel[i]);
		*trot = TurRot;
	//	tsteer = TurSteer;
	};
};

#define HISTORY_SIZE 32
//Must be pow2.

class Vehicle{
public:
	//New, network prediction history buffer...
	VehicleStateLump History[HISTORY_SIZE];
	int32_t CurrentLump;
	uint32_t CurrentTime;
	//
public:	//History based functions.
	void SetCurrentTime(uint32_t time);
	int32_t SetControlInput(uint32_t time, float laccel, float raccel, float turrotvel);
	int32_t SetAuthoritativeState(uint32_t time, Vec3 pos, Vec3 vel, Mat3 rot, Mat3 rotvel,
		float laccel, float raccel, float turrot, float turrotvel, int32_t ingbits);
	int32_t PredictCurrentTime(Vec3 posout, Rot3 rotout, Vec3 velout, float *turrot, Terrain *map);
	//
	//
	float LTurRot, TurRot;//, TurRotVel;
	//
//	Object Body, Wheel[4];
//	Vector LPos, Pos, LPosD, PosD;
//	Rotation LRot, Rot, LRotD, RotD;
	int32_t Type;
	CVec3 LPos, Pos;
	CVec3 Vel;
//	Vec3 LPosD, PosD;
//	Rot3 LRot, Rot, LRotD, RotD;
	Mat3 LRotM, RotM, LRotVelM, RotVelM;
	//
	CVec3 NPos, NVel;
	Mat3 NRotM, NRotVelM;
	int32_t UseNetworkNext;
	//Testing, Next Network Values.
	//
//	float Gravity,
	float Friction;
	Vec3 Gravity;
	float SteerSpeed, AccelSpeed, MaxSpeed;
//	Pnt3 lwv[10];
//	int32_t Wheels, WheelPoints;
//	int32_t ResetLwv;
	bool ConstrainRot;
	CVec3 BndMin, BndMax;
	//
//	float FrontOnGround, RearOnGround, OnGround;
//	float LeftOnGround, RightOnGround;
//	float ForwardVel, ForwardDot;
	//
//	double LastThinkTime;
	uint32_t LastThinkTime;
	float Fraction;
	int32_t FractionMS;
	//
	CVec3 WP[MAXWHEELPOINTS];
	CVec3 LastWP[MAXWHEELPOINTS];
	CVec3 AccelWP[MAXWHEELPOINTS];	//Holds the vehicle-relative meters per second accelerations of each wheel point32_t for this tick.
	bool InGround[MAXWHEELPOINTS];
	int32_t InGroundTotal;
	float LinearVelocity;
	int32_t InGroundBits;
	//
	int32_t WheelPoints, GroundWheelPoints;
	int32_t BackLeft, BackRight, FrontLeft, FrontRight;	//Indexes into WP arrays.
	//
	CVec3 AddAccelWP[MAXWHEELPOINTS];
	CVec3 AddAccel;	//Vehicular external velocity change.
	//
private:
public:
	Vehicle();
	Vehicle(float grav, float fric, bool conrot, int32_t type, float frac);
	~Vehicle();
	void oldThink(float Steer, float Accel, float Brake, float Scale, Terrain *terr);
	void Init(float grav, float fric, bool conrot, int32_t type, float frac);
	void SetGravity(Vec3 grav);
	void SetFriction(float fric);
	void Think(float LeftAccel, float RightAccel, uint32_t clocktime, Vec3 pos, Rot3 rot, Vec3 vel,
		float *turrot, float turrotvel, Terrain *map);
	void Think2(float LeftAccel, float RightAccel, float turrotvel, Terrain *map);
	void SetPos(Vec3 pos);
	void SetRot(Rot3 rot);
	void SetVel(Vec3 vel);
	void Config(float steer, float accel, float max);
public:
	virtual bool CheckCollision() { return false; };	//Override this in child class to check for and handle collisions every physics interval.
};

