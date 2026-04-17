/*
 * touch_handler.h
 *
 *  Created on: Apr 16, 2026
 *      Author: Lydia
 */

#ifndef TOUCH_HANDLER_H_
#define TOUCH_HANDLER_H_


typedef enum {
    TOUCH_GESTURE_NOTHING,

    TOUCH_GESTURE_SINGLE_FORWARD,
    TOUCH_GESTURE_SINGLE_BACKWARD,
    TOUCH_GESTURE_SINGLE_PRESS_HOLD,
    TOUCH_GESTURE_SINGLE_TAP,
    TOUCH_GESTURE_SINGLE_DOUBLE_TAP,
    TOUCH_GESTURE_SINGLE_TRIPLE_TAP,

    TOUCH_GESTURE_TWO_TAP,
    TOUCH_GESTURE_TWO_FORWARD,
    TOUCH_GESTURE_TWO_BACKWARD,
    TOUCH_GESTURE_TWO_PRESS_HOLD,
} touch_gesture_t;

void touch_gesture_handler (void);
void set_touch_gesture (touch_gesture_t touch_gesture);

#endif /* TOUCH_HANDLER_H_ */
