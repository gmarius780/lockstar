import React from 'react'

import Devices from '../../components/devices'

export default () => {
  return (
    <>
      <Devices devicesPath="/api/devices?type=led" />
    </>
  )
}
