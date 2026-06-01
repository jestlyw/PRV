#include <iostream>
#include <cuda_runtime.h>
#include <device_launch_parameters.h>
#include <cstdlib>
#include <cmath>
#include <chrono>

#define WIDTH  1024
#define HEIGHT 1024

void blurFilterCPU(unsigned char* input, unsigned char* output, int width, int height) {
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int sum = 0;
            int count = 0;
            for (int dy = -1; dy <= 1; dy++) {
                for (int dx = -1; dx <= 1; dx++) {
                    int ny = y + dy;
                    int nx = x + dx;
                    if (ny >= 0 && ny < height && nx >= 0 && nx < width) {
                        sum += input[ny * width + nx];
                        count++;
                    }
                }
            }
            output[y * width + x] = (unsigned char)(sum / count);
        }
    }
}

__global__ void blurFilterCUDA(unsigned char* input, unsigned char* output, int width, int height) {
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;

    if (x >= width || y >= height) return;

    int sum = 0;
    int count = 0;
    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            int ny = y + dy;
            int nx = x + dx;
            if (ny >= 0 && ny < height && nx >= 0 && nx < width) {
                sum += input[ny * width + nx];
                count++;
            }
        }
    }
    output[y * width + x] = (unsigned char)(sum / count);
}

int main() {
    size_t size = WIDTH * HEIGHT * sizeof(unsigned char);

    unsigned char* h_input      = new unsigned char[WIDTH * HEIGHT];
    unsigned char* h_output_cpu = new unsigned char[WIDTH * HEIGHT];
    unsigned char* h_output_gpu = new unsigned char[WIDTH * HEIGHT];

    for (int i = 0; i < WIDTH * HEIGHT; i++)
        h_input[i] = (unsigned char)(rand() % 256);

    // CPU
    auto cpu_start = std::chrono::high_resolution_clock::now();
    blurFilterCPU(h_input, h_output_cpu, WIDTH, HEIGHT);
    auto cpu_end = std::chrono::high_resolution_clock::now();
    double cpu_time = std::chrono::duration<double>(cpu_end - cpu_start).count();

    unsigned char *d_input, *d_output;
    cudaMalloc((void**)&d_input,  size);
    cudaMalloc((void**)&d_output, size);

    cudaMemcpy(d_input, h_input, size, cudaMemcpyHostToDevice);

    dim3 threadsPerBlock(16, 16);
    dim3 blocksPerGrid((WIDTH + 15) / 16, (HEIGHT + 15) / 16);

    // GPU
    auto gpu_start = std::chrono::high_resolution_clock::now();
    blurFilterCUDA<<<blocksPerGrid, threadsPerBlock>>>(d_input, d_output, WIDTH, HEIGHT);
    cudaDeviceSynchronize();
    auto gpu_end = std::chrono::high_resolution_clock::now();
    double gpu_time = std::chrono::duration<double>(gpu_end - gpu_start).count();

    cudaMemcpy(h_output_gpu, d_output, size, cudaMemcpyDeviceToHost);

    bool ok = true;
    for (int i = 0; i < WIDTH * HEIGHT; i++) {
        if (abs((int)h_output_cpu[i] - (int)h_output_gpu[i]) > 1) {
            ok = false;
            break;
        }
    }

    std::cout << "Результаты " << (ok ? "совпадают" : "не совпадают") << std::endl;
    std::cout << "CPU time: " << cpu_time << " s" << std::endl;
    std::cout << "GPU time: " << gpu_time << " s" << std::endl;
    std::cout << "Ускорение: " << cpu_time / gpu_time << "x" << std::endl;

    cudaFree(d_input);
    cudaFree(d_output);
    delete[] h_input;
    delete[] h_output_cpu;
    delete[] h_output_gpu;

    return 0;
}
