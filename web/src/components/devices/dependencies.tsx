import React, { useEffect, useCallback, useState } from 'react'
import Typography from '@material-ui/core/Typography'
import CircularProgress from '@material-ui/core/CircularProgress'
import { Theme, makeStyles, createStyles } from '@material-ui/core/styles'
import { green, red } from '@material-ui/core/colors'

import useLocalStorage from '../../hooks/useLocalStorage'
import Snackbar from '../snackbar'

const useStyles = makeStyles((theme: Theme) => createStyles({
  title: {
    marginBottom: theme.spacing(1),
  },
  wrapper: {
    margin: theme.spacing(1),
    position: 'relative',
  },
  buttonSuccess: {
    backgroundColor: green[500],
    '&:hover': {
      backgroundColor: green[700],
    },
  },
  buttonError: {
    backgroundColor: red[500],
    '&:hover': {
      backgroundColor: red[700],
    },
  },
  buttonProgress: {
    color: green[500],
  },
}))

interface Values {
  pin?: number
  label: string
  type: string
}

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

interface Props {
  device?: Device | null
}

interface DependencyItemProps {
  dependency: InnerDevice
}

const DependencyItem = ({ dependency }: DependencyItemProps) => {
  return (
    <>
      {dependency?.label}
    </>
  )
}

export default ({ device }: Props) => {
  const classes = useStyles()
  const [serverBaseUrl] = useLocalStorage('serverBaseUrl')
  const [data, setData] = useState<Device[] | null>(null)
  const [error, setError] = useState<string | null>(null)
  const [loading, setLoading] = useState(false)

  const fetchData = useCallback(async () => {
    setLoading(true)

    const url = device?.type === 'led' ? `${serverBaseUrl}/api/devices?type=button` : `${serverBaseUrl}/api/devices?type=led`
    const data = await fetch(url)

    if (data.ok) {
      setData(await data.json())
    } else {
      setError(await data.text())
    }

    setLoading(false)
  }, [serverBaseUrl, device])

  useEffect(() => {
    fetchData()
  }, [fetchData])

  return (
    <>
      <Snackbar message={error} severity="error" onClose={() => setError(null)}/>
      {!error ? loading ? <CircularProgress /> : data ? (
        <>
          <Typography variant="h5" className={classes.title} noWrap>
            Device Dependencies
          </Typography>

          {device?.dependencies && device?.dependencies.map(dependency => <DependencyItem key={dependency.pin} dependency={dependency} />)}
        </>
      ) : null : null}
    </>
  )
}
