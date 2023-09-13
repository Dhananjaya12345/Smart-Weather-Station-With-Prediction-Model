#!/usr/bin/env python
# coding: utf-8

# In[ ]:


import tensorflow as tf
from tensorflow import keras
import pandas as pd
import numpy as np
import requests  # Import requests library to make HTTP POST requests

# Importing the dataset
weather_dataset = pd.read_csv('7_Day_forcast.csv')
print(weather_dataset)

# Shuffling the data
np.random.shuffle(weather_dataset.values)
print(weather_dataset.values)

# Conversion of the date field into integer format
day_dic = {
    'San': 1,
    'Man': 2,
    'Tues': 3,
    'Wedn': 4,
    'Thur': 5,
    'Frid': 6,  # Added Friday as 6th day
    'Satur': 7  # Added Saturday as 7th day
}
print(day_dic)
print(weather_dataset['Day'])

# Formulating inputs
weather_inputs = np.column_stack((weather_dataset.Day.map(day_dic).values, weather_dataset.Time.values, weather_dataset.Temperature.values, weather_dataset.Humidity.values, weather_dataset.UV_level.values))
print(weather_inputs)

# Formulating outputs
weather_outputs = np.column_stack((weather_dataset.Rain.values, weather_dataset.Wind_Speed.values))
print(weather_outputs)

print()
print('Building the neural network')

# Preparing the neural network
weather_NN = keras.Sequential([
    keras.layers.Dense(128, input_shape=(5,), activation='relu'),
    keras.layers.Dense(64, activation='relu'),
    keras.layers.Dense(32, activation='relu'),
    keras.layers.Dense(16, activation='relu'),
    keras.layers.Dense(8, activation='relu'),
    keras.layers.Dense(2)  # Output layer with linear activation
])

# Compiling the neural network
weather_NN.compile(optimizer='adam', loss='mean_squared_error', metrics=['accuracy'])

# Fitting the model
history = weather_NN.fit(weather_inputs.astype(np.float32), weather_outputs.astype(np.float32), batch_size=4, epochs=10, validation_split=0.2, verbose=2)

# Evaluate the model on the test data
loss, accuracy = weather_NN.evaluate(weather_inputs.astype(np.float32), weather_outputs.astype(np.float32))
print('Test Accuracy:', accuracy)

# Predicting the rain and wind speed
prediction = weather_NN.predict(np.column_stack((np.array([5]), np.array([175510]), np.array([28.1]), np.array([90.2]), np.array([30]))))

# Apply threshold for rain prediction
rain_threshold = 0.5
binary_rain_prediction = 1 if prediction[0][0] >= rain_threshold else 0

print('Prediction - Rain (Binary):', binary_rain_prediction)  # 0 or 1
print('Prediction - Wind Speed:', prediction[0][1])  # Predicted wind speed

# Send data to ThingSpeak
api_key = '0II1YQGDO1YRYQ87'
base_url = f'https://api.thingspeak.com/update?api_key={api_key}'
data = {
    'field1': binary_rain_prediction,
    'field2': prediction[0][1]
}

response = requests.post(base_url, params=data)

if response.status_code == 200:
    print('Data sent to ThingSpeak successfully!')
else:
    print('Error sending data to ThingSpeak. Status Code:', response.status_code)


# In[ ]:




