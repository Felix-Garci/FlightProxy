import pygame
import sys

pygame.init()
pygame.joystick.init()

count = pygame.joystick.get_count()
print(f"---- DIAGNÓSTICO ----")
print(f"Dispositivos encontrados: {count}")

if count == 0:
    print("ERROR: Pygame no ve ningún dispositivo.")
else:
    for i in range(count):
        try:
            joy = pygame.joystick.Joystick(i)
            joy.init()
            print(f"ID {i}: {joy.get_name()}")
            print(f"   -> Ejes: {joy.get_numaxes()}")
            print(f"   -> Botones: {joy.get_numbuttons()}")
            joy.quit()
        except Exception as e:
            print(f"ID {i}: Error al leer ({e})")

pygame.quit()
