import math
import numpy as np

# Define sinus wave parameters
num_samples = 1000
amplitude = np.float32(1.0)
frequency = np.float32(1.0)
x_values = np.float32(np.linspace(0.5, -0.5, num_samples+1))

# Generate y values
y_values = np.float32([amplitude * math.sin(np.pi * frequency * x) for x in x_values])

# Open C header file
with open('data/angle_data.h', 'w') as f:
    # Write includes and define statements
    f.write('#ifndef GEN_DATA_H\n')
    f.write('#define GEN_DATA_H\n\n')
    f.write(f'#define ARRAY_SIZE {num_samples}\n\n')
    f.write('const uint32_t sin_values[ARRAY_SIZE] = {\n')

    # Write sinus wave values
    index = 0
    for y in y_values[:-1]:
        if y < 0:
            y = 2 + y
        index += 1
        # f.write(f'{y}, ')
        f.write(f'{hex(int(y*(2**31)))}, ')
        # f.write(f'{float(f"{y:.23g}"):g}, ')
        if(index % 8 == 0):
            f.write('\n')
    f.write('};\n\n\n')
    f.write('const uint32_t angle_values[ARRAY_SIZE] = {\n')
    index = 0
    for x in x_values[:-1]:
        if x < 0:
            x = 2 + x
        index += 1
        f.write(f'{hex(int(x*(2**31)))}, ')
        if(index % 8 == 0):
            f.write('\n')


    f.write('};\n\n#endif /* GEN_DATA_H */\n')
