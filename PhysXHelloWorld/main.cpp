#include <ctype.h>
#include <iostream>
#include <chrono>
#include <thread>
#include "PxPhysicsAPI.h"
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

    // Set up a default filter shader (you can replace this with your own filter shader if needed)
    sceneDesc.filterShader = PxDefaultSimulationFilterShader;

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
    scene->addActor(*groundPlane);

    // Create stacked boxes
    PxMaterial* boxMaterial = physics->createMaterial(0.5f, 0.5f, 0.1f);
    float boxHalfExtent = 1.0f;
    float boxSpacing = 2.0f;

    for (int i = 0; i < 5; ++i) {
        PxTransform boxTransform(PxVec3(0.0f, i * (2 * boxHalfExtent + boxSpacing), 0.0f));
        PxBoxGeometry boxGeometry(PxVec3(boxHalfExtent, boxHalfExtent, boxHalfExtent));
        PxRigidDynamic* box = PxCreateDynamic(*physics, boxTransform, boxGeometry, *boxMaterial, 1.0f);
        scene->addActor(*box);
    }

    std::chrono::steady_clock::time_point lastFrameTime = std::chrono::high_resolution_clock::now();
    
    const float timeStep = 1.0f / 30.0f;
    while (true) {
        // Process input, update game state, etc. (you can add your game logic here)

        // Simulate the scene
        scene->simulate(timeStep);
        scene->fetchResults(true);


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
    scene->release();
    physics->release();
    mFoundation->release();

    return 0;
}