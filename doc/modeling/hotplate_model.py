import numpy as np

def interpolate(x, points):
    if len(points) < 2:
        raise ValueError("At least two points are required for interpolation.")

    if x <= points[0][0]:
        x1, y1 = points[0]
        x2, y2 = points[1]
        return y1 + (x - x1) * (y2 - y1) / (x2 - x1)

    if x >= points[-1][0]:
        x1, y1 = points[-2]
        x2, y2 = points[-1]
        return y1 + (x - x1) * (y2 - y1) / (x2 - x1)

    for i in range(1, len(points)):
        if x < points[i][0]:
            x1, y1 = points[i-1]
            x2, y2 = points[i]
            return y1 + (x - x1) * (y2 - y1) / (x2 - x1)

    raise ValueError("Interpolation error: x is out of bounds.")

class HotplateModel:
    TUNGSTEN_TC = 0.0041  # Temperature coefficient for tungsten

    def __init__(self, x=0.08, y=0.07, z=0.0038):
        self.size = {'x': x, 'y': y, 'z': z}  # Plate dimensions in meters
        self.calibration_points = []  # Calibration points for resistance and heat dissipation
        self.max_current = 5  # Max current in Amps
        self.max_voltage = 20  # Max voltage in Volts
        self.temperature = None  # Current temperature
        self.name = ""  # Label for the hotplate
        # By default rely on clamping limits to simplify configuration
        self.power_setpoint = 1000

    def clone(self):
        # Create a new instance with the same properties
        new_instance = HotplateModel(
            x=self.size['x'], 
            y=self.size['y'], 
            z=self.size['z']
        )
        new_instance.calibration_points = [point.copy() for point in self.calibration_points]
        new_instance.max_current = self.max_current
        new_instance.max_voltage = self.max_voltage
        new_instance.temperature = self.temperature
        new_instance.name = self.name
        new_instance.power_setpoint = self.power_setpoint
        return new_instance

    def reset(self):
        # Reset temperature to the initial calibration point or room temperature
        self.temperature = self.get_room_temp()
        return self

    def clamp_current(self, current):
        self.max_current = current
        return self

    def clamp_voltage(self, voltage):
        self.max_voltage = voltage
        return self

    def label(self, name):
        self.name = name
        return self

    def set_size(self, x, y, z):
        self.size = {'x': x, 'y': y, 'z': z}
        return self

    def scale_r_to(self, new_base):
        # Scale the resistance in all calibration points, to simplify configuration for different heaters
        ratio = new_base / self.calibration_points[0]['R']
        for point in self.calibration_points:
            point['R'] *= ratio
        return self

    def calibrate(self, T, R=None, W=None, V=None, I=None):
        if R is not None and W is None and V is None and I is None:
            # Case: {T, R}
            self.calibration_points.append({'T': T, 'R': R, 'W': 0})
            self.temperature = T  # Set initial temperature based on the room temperature calibration point
        elif W is not None and R is not None and V is None and I is None:
            # Case: {T, R, W}
            self.calibration_points.append({'T': T, 'R': R, 'W': W})
        elif W is not None and V is not None and R is None and I is None:
            # Case: {T, W, V}
            self.calibration_points.append({'T': T, 'R': V**2 / W, 'W': W})
        elif W is not None and I is not None and R is None and V is None:
            # Case: {T, W, I}
            self.calibration_points.append({'T': T, 'R': W / (I**2), 'W': W})
        else:
            raise ValueError("Invalid combination of calibration inputs. Must provide either {T, R} or {T, W, R | V | I}.")

        # Sort calibration points by temperature and check that resistance is non-decreasing
        self.calibration_points.sort(key=lambda point: point['T'])

        # Minimal validation
        for i in range(1, len(self.calibration_points)):
            if self.calibration_points[i]['R'] < self.calibration_points[i-1]['R']:
                raise ValueError("Calibration points must have non-decreasing resistance values for increasing temperatures.")

        return self

    def iterate(self, dt):
        clamped_power = self.get_power()
        
        heat_capacity = self.calculate_heat_capacity()
        heat_transfer_coefficient = self.calculate_heat_transfer_coefficient()
        temperature_change = (clamped_power - heat_transfer_coefficient * (self.temperature - self.get_room_temp())) * dt / heat_capacity
        
        self.temperature += temperature_change

    def calculate_resistance(self, temperature):
        if len(self.calibration_points) == 0:
            raise ValueError("No calibration points defined.")

        if len(self.calibration_points) == 1:
            single_point = self.calibration_points[0]
            return single_point['R'] * (1 + self.TUNGSTEN_TC * (temperature - single_point['T']))

        points = [(p['T'], p['R']) for p in self.calibration_points]
        return interpolate(temperature, points)

    def calculate_heat_capacity(self):
        material_shc = 897  # J/kg/K for Aluminum 6061
        material_density = 2700  # kg/m3 for Aluminum 6061
        volume = self.size['x'] * self.size['y'] * self.size['z']  # in m³
        mass = volume * material_density  # in kg
        return mass * material_shc

    def calculate_heat_transfer_coefficient(self):
        points_with_power = [p for p in self.calibration_points if p['W'] != 0]

        if not points_with_power:
            area = self.size['x'] * self.size['y']  # in m²
            return 40 * area  # in W/K, Default empiric value, when no calibration data is available

        if len(points_with_power) == 1:
            point = points_with_power[0]
            return point['W'] / (point['T'] - self.get_room_temp())

        points = [(p['T'], p['W'] / (p['T'] - self.get_room_temp())) for p in points_with_power]
        return interpolate(self.temperature, points)

    def set_power(self, power):
        if (power < 0): self.power_setpoint = 0
        else: self.power_setpoint = power

        return self
    
    def get_max_power(self):
        R = self.calculate_resistance(self.temperature)
        V_max_power = self.max_voltage ** 2 / R
        I_max_power = self.max_current ** 2 * R

        return min(V_max_power, I_max_power)

    def get_power(self):
        return min(self.get_max_power(), self.power_setpoint)
    
    def get_resistance(self):
        return self.calculate_resistance(self.temperature)

    def get_room_temp(self):
        room_temp_point = next((p for p in self.calibration_points if p['W'] == 0), None)
        return room_temp_point['T'] if room_temp_point else 25  # Default to 25°C if no room temperature point is found
