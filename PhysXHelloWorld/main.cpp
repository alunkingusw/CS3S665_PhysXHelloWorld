#include <ctype.h>
#include <iostream>
#include <chrono>
#include <thread>
#include "PxPhysicsAPI.h"
#include "SoundEventCallback.h"
#include "CharacterController.h"
using namespace physx;

PxPvd* mPvd = NULL;

int main() {
    static PxDefaultErrorCallback gDefaultErrorCallback;
    static PxDefaultAllocator gDefaultAllocatorCallback;

    PxFoundation* mFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, gDefaultAllocatorCallback,
        gDefaultErrorCallback);
    if (!mFoundation) {
        std::cerr << "Error creating PhysX foundation." << std::endl;
    }

    //setup Pvd manager
    mPvd = PxCreatePvd(*mFoundation);
    physx::PxPvdTransport* transport = physx::PxDefaultPvdSocketTransportCreate("127.0.0.1", 5425, 10);
    mPvd->connect(*transport, physx::PxPvdInstrumentationFlag::eALL);

    // Physics
    PxPhysics* physics = PxCreatePhysics(PX_PHYSICS_VERSION, *mFoundation, PxTolerancesScale(), true, mPvd);
    if (!physics) {
        std::cerr << "Error creating PhysX physics." << std::endl;
        mFoundation->release();
        return 1;
    }

    // Scene
    PxSceneDesc sceneDesc(physics->getTolerancesScale());
    sceneDesc.gravity = PxVec3(0.0f, -9.81f, 0.0f);

    sceneDesc.flags |= PxSceneFlag::eENABLE_ACTIVE_ACTORS;

    // Set up a default CPU dispatcher
    PxDefaultCpuDispatcher* cpuDispatcher = PxDefaultCpuDispatcherCreate(1);  // 2 threads
    sceneDesc.cpuDispatcher = cpuDispatcher;

    //create collision callback instance
    SoundEventCallback collisionCallback;

    // Set up the SoundEventCallback as our filter shader
    sceneDesc.filterShader = &boxCollisionFilterShader;
    sceneDesc.simulationEventCallback = &collisionCallback;

    PxScene* scene = physics->createScene(sceneDesc);

    //add Pvd flags
    physx::PxPvdSceneClient* pvdClient = scene->getScenePvdClient();
    if (pvdClient)
    {
        pvdClient->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
        pvdClient->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
        pvdClient->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);
    }

    if (!scene) {
        std::cerr << "Error creating PhysX scene." << std::endl;
        if (cpuDispatcher) cpuDispatcher->release();
        
        physics->release();
        mFoundation->release();
        return 1;
    }

   
    // Create ground plane
    PxMaterial* groundMaterial = physics->createMaterial(0.5f, 0.5f, 0.1f);
    PxTransform groundTransform = PxTransform(PxQuat(PxHalfPi, PxVec3(0, 0, 1)));
    PxRigidStatic* groundPlane = PxCreateStatic(*physics, groundTransform, PxPlaneGeometry(), *groundMaterial);
    setupFiltering(groundPlane, FilterGroup::eFLOOR, FilterGroup::eBOX);
    scene->addActor(*groundPlane);

    // Create stacked boxes
    PxMaterial* boxMaterial = physics->createMaterial(0.5f, 0.5f, 0.1f);
    float boxHalfExtent = 1.0f;
    float boxSpacing = 2.0f;

    for (int i = 0; i < 5; ++i) {
        PxTransform boxTransform(PxVec3(0.0f, i * (2 * boxHalfExtent + boxSpacing), 0.0f));
        PxBoxGeometry boxGeometry(PxVec3(boxHalfExtent, boxHalfExtent, boxHalfExtent));
        PxRigidDynamic* box = PxCreateDynamic(*physics, boxTransform, boxGeometry, *boxMaterial, 1.0f);
        box->setActorFlag(PxActorFlag::eSEND_SLEEP_NOTIFIES, true);
        setupFiltering(box, FilterGroup::eBOX, FilterGroup::eBOX | FilterGroup::eFLOOR);
        //setupFiltering(box, FilterGroup::eBOX, FilterGroup::eFLOOR);
        scene->addActor(*box);
    }

    //create a controller manager
    PxControllerManager* controllerManager = PxCreateControllerManager(*scene);

    // Create Character instance
    CharacterController character(controllerManager, scene);


    // Call createCharacter to initialize the character
    physx::PxMaterial* characterMaterial = physics->createMaterial(0.5f, 0.5f, 0.1f);

    character.createCharacter(physx::PxExtendedVec3(5.0, 0.5, 5.0), characterMaterial);
    //scene->addActor(character);

    std::chrono::steady_clock::time_point lastFrameTime = std::chrono::high_resolution_clock::now();
    
    const float timeStep = 1.0f / 60.0f;
    const float slowMotionFactor = 0.05f;
    while (true) {
        // Process input, update game state, etc. (you can add your game logic here)

        // Simulate the scene
        scene->simulate(timeStep*slowMotionFactor);
        scene->fetchResults(true);
        
        character.update();

        // Add a sleep or some form of synchronization to control the frame rate
        std::chrono::duration<float> elapsedTime = std::chrono::duration_cast<std::chrono::duration<float>>(std::chrono::high_resolution_clock::now() - lastFrameTime);
        // (Note: This is a simple example and may not be suitable for more complex games)
        if (elapsedTime.count() < timeStep) {
            std::this_thread::sleep_for(std::chrono::duration<float>(timeStep - elapsedTime.count()));
        }

        //update the last frame time
        lastFrameTime = std::chrono::high_resolution_clock::now();

    }

    // Clean up
    controllerManager->purgeControllers();
    scene->release();
    physics->release();
    mFoundation->release();

    return 0;
}