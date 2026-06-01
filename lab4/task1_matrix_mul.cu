#include <iostream>
#include <cuda_runtime.h>
#include <device_launch_parameters.h>
#include <cstdlib>
#include <cmath>
#include <chrono>

#define M 512
#define N 512
#define P 512

void matrixMultiplyCPU(float* A, float* B, float* C, int m, int n, int p) {
    for (int row = 0; row < m; row++) {
        for (int col = 0; col < p; col++) {
            float sum = 0.0f;
            for (int k = 0; k < n; k++) {
                sum += A[row * n + k] * B[k * p + col];
            }
            C[row * p + col] = sum;
        }
    }
}

__global__ void matrixMultiplyCUDA(float* A, float* B, float* C, int m, int n, int p) {
    int row = blockIdx.y * blockDim.y + threadIdx.y;
    int col = blockIdx.x * blockDim.x + threadIdx.x;

    if (row < m && col < p) {
        float sum = 0.0f;
        for (int k = 0; k < n; k++) {
            sum += A[row * n + k] * B[k * p + col];
        }
        C[row * p + col] = sum;
    }
}

int main() {
    size_t sizeA = M * N * sizeof(float);
    size_t sizeB = N * P * sizeof(float);
    size_t sizeC = M * P * sizeof(float);

    float* h_A = new float[M * N];
    float* h_B = new float[N * P];
    float* h_C_cpu = new float[M * P];
    float* h_C_gpu = new float[M * P];

    for (int i = 0; i < M * N; i++) h_A[i] = static_cast<float>(rand() % 10);
    for (int i = 0; i < N * P; i++) h_B[i] = static_cast<float>(rand() % 10);

    // CPU
    auto cpu_start = std::chrono::high_resolution_clock::now();
    matrixMultiplyCPU(h_A, h_B, h_C_cpu, M, N, P);
    auto cpu_end = std::chrono::high_resolution_clock::now();
    double cpu_time = std::chrono::duration<double>(cpu_end - cpu_start).count();

    float *d_A, *d_B, *d_C;
    cudaMalloc((void**)&d_A, sizeA);
    cudaMalloc((void**)&d_B, sizeB);
    cudaMalloc((void**)&d_C, sizeC);

    cudaMemcpy(d_A, h_A, sizeA, cudaMemcpyHostToDevice);
    cudaMemcpy(d_B, h_B, sizeB, cudaMemcpyHostToDevice);

    dim3 threadsPerBlock(16, 16);
    dim3 blocksPerGrid((P + 15) / 16, (M + 15) / 16);

    // GPU
    auto gpu_start = std::chrono::high_resolution_clock::now();
    matrixMultiplyCUDA<<<blocksPerGrid, threadsPerBlock>>>(d_A, d_B, d_C, M, N, P);
    cudaDeviceSynchronize();
    auto gpu_end = std::chrono::high_resolution_clock::now();
    double gpu_time = std::chrono::duration<double>(gpu_end - gpu_start).count();

    cudaMemcpy(h_C_gpu, d_C, sizeC, cudaMemcpyDeviceToHost);

    bool ok = true;
    for (int i = 0; i < M * P; i++) {
        if (std::abs(h_C_cpu[i] - h_C_gpu[i]) > 1e-1f) {
            ok = false;
            break;
        }
    }

    std::cout << "Результаты " << (ok ? "совпадают" : "не совпадают") << std::endl;
    std::cout << "CPU time: " << cpu_time << " s" << std::endl;
    std::cout << "GPU time: " << gpu_time << " s" << std::endl;
    std::cout << "Ускорение: " << cpu_time / gpu_time << "x" << std::endl;

    cudaFree(d_A);
    cudaFree(d_B);
    cudaFree(d_C);
    delete[] h_A;
    delete[] h_B;
    delete[] h_C_cpu;
    delete[] h_C_gpu;

    return 0;
}
