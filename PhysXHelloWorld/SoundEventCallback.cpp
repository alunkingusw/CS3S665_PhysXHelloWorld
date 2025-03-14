#include "SoundEventCallback.h" 


void SoundEventCallback::onContact(const physx::PxContactPairHeader& pairHeader, const physx::PxContactPair* pairs, physx::PxU32 nbPairs) {
	// Iterate through contact pairs
	for (physx::PxU32 i = 0; i < nbPairs; i++) {
		const physx::PxContactPair& cp = pairs[i];
		// Handle collision events as needed 
		if (cp.events & physx::PxPairFlag::eNOTIFY_TOUCH_FOUND) {
			//get the actors involved
			physx::PxActor* actorA = pairHeader.actors[0];
			physx::PxActor* actorB = pairHeader.actors[1];
			FilterGroup::Enum typeA = static_cast<FilterGroup::Enum>(reinterpret_cast<uintptr_t>(actorA->userData));
			FilterGroup::Enum typeB = static_cast<FilterGroup::Enum>(reinterpret_cast<uintptr_t>(actorB->userData));
			//get their filter data to compare with BOX or FLOOR
			//physx::PxFilterData filterDataA = actorA->getSimulationFilterData();
			//physx::PxFilterData filterDataB = actorB->getSimulatiItonFilterData();
			if (typeA == FilterGroup::eBOX && typeB == FilterGroup::eBOX) {
				std::cout << "Box hit another Box!" << std::endl;
			}
			else if ((typeA == FilterGroup::eBOX && typeB == FilterGroup::eFLOOR) ||
				(typeA == FilterGroup::eFLOOR && typeB == FilterGroup::eBOX)) {
				std::cout << "Box hit the Floor!" << std::endl;
			}
			else if ((typeA == FilterGroup::eBOX && typeB == FilterGroup::eCHARACTER) ||
				(typeA == FilterGroup::eCHARACTER && typeB == FilterGroup::eBOX)) {
				std::cout << "Character hit the Box!" << std::endl;
			}
			else {
				std::cout << "Unknown collision detected!" << std::endl;
			}
		}
	}
}

void SoundEventCallback::onTrigger(physx::PxTriggerPair* pairs, physx::PxU32 count){
	for (physx::PxU32 i = 0; i < count; i++) {
		const physx::PxTriggerPair& triggerPair = pairs[i];
		if (triggerPair.status == physx::PxPairFlag::eNOTIFY_TOUCH_FOUND) {
			std::cout << "Trigger detected!" << std::endl;
		}
	}
}

void SoundEventCallback::onWake(physx::PxActor** actors, physx::PxU32 count){
	for (physx::PxU32 i = 0; i < count; i++) {
		std::cout << "Actor woke up!" << std::endl;
	}
}

void SoundEventCallback::onSleep(physx::PxActor** actors, physx::PxU32 count) {
	for (physx::PxU32 i = 0; i < count; i++) {
		std::cout << "Actor went to sleep!" << std::endl;
	}
}

void SoundEventCallback::onAdvance(const physx::PxRigidBody* const* bodyBuffer, const physx::PxTransform* poseBuffer, const physx::PxU32 count) {
	for (physx::PxU32 i = 0; i < count; i++) {
		const physx::PxRigidBody* body = bodyBuffer[i];
		const physx::PxTransform& pose = poseBuffer[i];
		std::cout << "Advanced stage of rigid body!" << std::endl;
	}
}

void SoundEventCallback::onConstraintBreak(physx::PxConstraintInfo* constraints, physx::PxU32 count) {
	for (physx::PxU32 i = 0; i < count; i++) {
		const physx::PxConstraintInfo& constraintInfo = constraints[i];
		std::cout << "Constraint broken!" << std::endl;
	}
}


physx::PxFilterFlags boxCollisionFilterShader(
	physx::PxFilterObjectAttributes attributes0, physx::PxFilterData filterData0, 
	physx::PxFilterObjectAttributes attributes1, physx::PxFilterData filterData1,
	physx::PxPairFlags& pairFlags, const void* constantBlock, physx::PxU32 constantBlockSize) {

	// let triggers through
	if (physx::PxFilterObjectIsTrigger(attributes0) || physx::PxFilterObjectIsTrigger(attributes1)) {
		pairFlags = physx::PxPairFlag::eTRIGGER_DEFAULT; return physx::PxFilterFlag::eDEFAULT;
	}

	// generate contacts for all that were not filtered above
	pairFlags = physx::PxPairFlag::eCONTACT_DEFAULT;

	// trigger the contact callback for pairs (A,B) where
	// the filtermask of A contains the ID of B and vice versa.
	if ((filterData0.word0 & filterData1.word1) || (filterData1.word0 & filterData0.word1)) {
	
		pairFlags |= physx::PxPairFlag::eNOTIFY_TOUCH_FOUND;
	}
	return physx::PxFilterFlag::eDEFAULT;
}


void setupFiltering(physx::PxRigidActor* actor, physx::PxU32 filterGroup, physx::PxU32 filterMask) {
	physx::PxFilterData filterData; 
	filterData.word0 = filterGroup; // word0 = own ID
	filterData.word1 = filterMask;// word1 = ID mask to filter pairs that trigger a
	// contact callback (list multiple using |);

	const physx::PxU32 numShapes = actor->getNbShapes();

	actor->userData = reinterpret_cast<void*>(static_cast<uintptr_t>(filterGroup));
	
	// Allocate an array of PxShape* pointers.
	physx::PxShape** shapes = new physx::PxShape * [numShapes];
	// Get the shapes of the actor.
	actor->getShapes(shapes, numShapes);
	// Set the simulation filter data for each shape.
	for (physx::PxU32 i = 0; i < numShapes; ++i) {
		shapes[i]->setSimulationFilterData(filterData);
	}

	// Free the array of PxShape* pointers.
	delete[] shapes;
}
