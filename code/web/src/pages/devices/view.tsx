import React, { useEffect, useCallback, useState } from 'react'
import { useParams, useHistory } from 'react-router-dom'
import CircularProgress from '@material-ui/core/CircularProgress'
import { Theme, createStyles, makeStyles } from '@material-ui/core/styles'
import EditIcon from '@material-ui/icons/Edit'
import DeleteIcon from '@material-ui/icons/Delete'
import SpeedDial from '@material-ui/lab/SpeedDial'
import SpeedDialIcon from '@material-ui/lab/SpeedDialIcon'
import SpeedDialAction from '@material-ui/lab/SpeedDialAction'

import useLocalStorage from '../../hooks/useLocalStorage'
import Snackbar from '../../components/snackbar'

interface Device {
  pin: number
  label: string
  type: string
  status?: number
}

const useStyles = makeStyles((theme: Theme) => createStyles({
  speedDial: {
    top: 'auto',
    right: theme.spacing(2),
    bottom: theme.spacing(2),
    left: 'auto',
    position: 'fixed',
  },
}))

export default () => {
  const classes = useStyles()
  const history = useHistory()
  const { deviceId } = useParams()
  const [serverBaseUrl] = useLocalStorage('serverBaseUrl')
  const [data, setData] = useState<Device | null>(null)
  const [error, setError] = useState<string | null>(null)
  const [loading, setLoading] = useState(false)
  const [speedDialOpen, setSpeedDialOpen] = useState(false)

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

  const handleDeleteDevice = async () => {
    setSpeedDialOpen(false)
    // TODO: display dialog
    // @ts-ignore TS7022
    const result = await fetch(`${serverBaseUrl}/api/devices/${data?.pin}`, {
      method: 'DELETE',
      headers: {
        'Content-Type': 'application/json',
      },
    })

    if (result.ok) {
      setTimeout(() => {
        history.push('/devices')
      }, 750)
    } else {
      setError(await result.text())
    }
  }

  return (
    <>
      <Snackbar message={error} severity="error" onClose={() => setError(null)}/>
      {!error ? loading ? <CircularProgress /> : data ? (
        <>
          view device {data?.pin}
          <SpeedDial
            ariaLabel="Actions"
            className={classes.speedDial}
            icon={<SpeedDialIcon />}
            onClose={() => setSpeedDialOpen(false)}
            onOpen={() => setSpeedDialOpen(true)}
            open={speedDialOpen}
          >
            <SpeedDialAction
              key={'edit'}
              icon={<EditIcon />}
              tooltipTitle={'Edit'}
              onClick={() => history.push(`/devices/edit/${deviceId}`)}
            />
            <SpeedDialAction
              key={'delete'}
              icon={<DeleteIcon />}
              tooltipTitle={'Delete'}
              onClick={handleDeleteDevice}
            />
          </SpeedDial>
        </>
      ) : null : null}
    </>
  )
}
