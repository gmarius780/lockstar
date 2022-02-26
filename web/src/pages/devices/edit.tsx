import React, { useEffect, useCallback, useState } from 'react'
import { useParams } from 'react-router-dom'
import CircularProgress from '@material-ui/core/CircularProgress'
import Box from '@material-ui/core/Box'

import useLocalStorage from '../../hooks/useLocalStorage'
import Snackbar from '../../components/snackbar'
import DeviceSettings from '../../components/devices/settings'
import DeviceDependencies from '../../components/devices/dependencies'

interface InnerDevice {
  pin: number
  label: string
  type: string
}

interface Device {
  pin: number
  label: string
  type: string
  dependencies: InnerDevice[]
  status?: number
}

export default () => {
  const { deviceId } = useParams()
  const [serverBaseUrl] = useLocalStorage('serverBaseUrl')
  const [data, setData] = useState<Device | null>(null)
  const [error, setError] = useState<string | null>(null)
  const [loading, setLoading] = useState(false)

  const fetchData = useCallback(async () => {
    setLoading(true)

    const data = await fetch(`${serverBaseUrl}/api/devices/${deviceId}`)

    if (data.ok) {
      setData(await data.json())
    } else {
      setError(await data.text())
    }

    setLoading(false)
  }, [serverBaseUrl, deviceId])

  useEffect(() => {
    fetchData()
  }, [fetchData])

  return (
    <>
      <Snackbar message={error} severity="error" onClose={() => setError(null)}/>
      {!error ? loading ? <CircularProgress /> : data ? (
        <>
          <DeviceSettings device={data} />
          <Box p={2} />
          <DeviceDependencies device={data} />
        </>
      ) : null : null}
    </>
  )
}
