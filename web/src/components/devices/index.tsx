import React, { useEffect, useCallback, useState } from 'react'
import { Link } from 'react-router-dom'
import { Theme, createStyles, makeStyles } from '@material-ui/core/styles'
import CircularProgress from '@material-ui/core/CircularProgress'
import grey from '@material-ui/core/colors/grey'
import Grid from '@material-ui/core/Grid'
import Card from '@material-ui/core/Card'
import CardActionArea from '@material-ui/core/CardActionArea'
import CardActions from '@material-ui/core/CardActions'
import CardContent from '@material-ui/core/CardContent'
import Button from '@material-ui/core/Button'
import Typography from '@material-ui/core/Typography'
import ToggleOnIcon from '@material-ui/icons/ToggleOn'
import ToggleOffIcon from '@material-ui/icons/ToggleOff'
import EmojiObjectsIcon from '@material-ui/icons/EmojiObjects'

import Snackbar from '../snackbar'
import useLocalStorage from '../../hooks/useLocalStorage'
import { initiateSocket, disconnectSocket, subscribeTo } from '../../utils/socket'

interface Device {
  pin: number
  label: string
  type: string
  status?: number
}

interface Props {
  devicesPath: string
}

interface DeviceListProps {
  devices: Device[] | null
}

interface DeviceItemProps {
  device: Device
}

interface MessageData {
  pin?: number
  status?: boolean
}

interface Message {
  eventName: string
  data: MessageData
}

const useStyles = makeStyles((theme: Theme) => createStyles({
  root: {
    flexGrow: 1,
  },
  card: {
    textAlign: 'center',
    color: theme.palette.text.secondary,
  },
  cardActions: {
    height: 50,
  },
  cardActionsRoot: {
    padding: 8,
    display: 'inline-flex',
  },
  media: {
    height: 140,
    width: 140,
    display: 'block',
    margin: 'auto',
  },
  actionItem: {
    display: 'inline-block',
  },
  deviceLabel: {
    color: grey[700],
  },
  link: {
    textDecoration: 'none',
    color: grey[500],
  },
}))

const DeviceItem = ({ device }: DeviceItemProps) => {
  const classes = useStyles()
  const [deviceStatus, setDeviceStatus] = useState(device.status)
  const [serverBaseUrl] = useLocalStorage('serverBaseUrl')

  // force re-render
  if (deviceStatus !== device.status) {
    setDeviceStatus(device.status)
  }

  const onButtonClick = async (statusToSet: number) => {
    if (deviceStatus === statusToSet) {
      const res = await fetch(`${serverBaseUrl}/api/devices/change-status`, {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
        },
        body: JSON.stringify({
          'pin': device.pin,
        }),
      })
      const { status } = await res.json()
      setDeviceStatus(status)
    }
  }

  let icon = null
  let iconStyles = { fontSize: 125 }
  let actions = null
  if (device.type === 'led') {
    if (deviceStatus) {
      // @ts-ignore TS2322
      iconStyles = { ...iconStyles, fill: '#ffc300' }
    }
    icon = <EmojiObjectsIcon style={iconStyles} />
    actions = (
      <>
        <Button size="small" color="primary" className={classes.actionItem} onClick={() => onButtonClick(deviceStatus || 0)}>
          {deviceStatus ? 'Turn Off' : 'Turn On'}
        </Button>
      </>
    )
  } else if (device.type.toLowerCase().endsWith('button')) {
    if (deviceStatus) {
      icon = <ToggleOnIcon style={{ ...iconStyles, fill: '#ffc300' }} />
    } else {
      icon = <ToggleOffIcon style={iconStyles} />
    }
  }

  return (
    <Grid item lg={3} md={4} xs={6}>
      <Card className={classes.card}>
        <CardActionArea>
          <Link to={`/devices/view/${device.pin}`} className={classes.link}>
            <>
              {icon}
              <CardContent>
                <Typography gutterBottom variant="h5" component="h2" className={classes.deviceLabel}>
                  {device.label}
                </Typography>
              </CardContent>
            </>
          </Link>
        </CardActionArea>
        <CardActions className={classes.cardActions} classes={{ root: classes.cardActionsRoot }}>
          {actions}
        </CardActions>
      </Card>
    </Grid>
  )
}

const DeviceList = ({ devices }: DeviceListProps) => {
  const classes = useStyles()

  return (
    <div className={classes.root}>
      <Grid container spacing={3}>
        {devices && devices.map(device => <DeviceItem key={device.pin} device={device} />)}
      </Grid>
    </div>
  )
}

export default ({ devicesPath }: Props) => {
  const [serverBaseUrl] = useLocalStorage('serverBaseUrl')
  const [message, setMessage] = useState<Message | null>(null)
  const [data, setData] = useState<Device[] | null>(null)
  const [error, setError] = useState<string | null>(null)
  const [loading, setLoading] = useState(false)

  const fetchData = useCallback(async () => {
    setMessage(null)
    setLoading(true)

    const data = await fetch(`${serverBaseUrl}${devicesPath}`)

    if (data.ok) {
      setData(await data.json())
    } else {
      setError(await data.text())
    }

    setLoading(false)
  }, [serverBaseUrl, devicesPath])

  useEffect(() => {
    fetchData()

    if (serverBaseUrl) {
      initiateSocket(`${serverBaseUrl}`)

      subscribeTo('all', (eventName: string, data: object) => setMessage({ eventName, data }))
    }

    return () => {
      disconnectSocket()
    }
  }, [fetchData, serverBaseUrl])

  if (message && data) {
    switch (message.eventName) {
    case 'deviceStatusChanged':
      const devices = data.map((device) => {
        if (device.pin === message.data.pin) {
          return {
            ...device,
            status: message.data.status,
          } as Device
        }
        return device
      }) as Device[]

      setData(devices)
      setMessage(null)
      break
    default:
      break
    }
  }

  return (
    <>
      <Snackbar message={error} severity="error" onClose={() => setError(null)}/>
      {!error ? loading ? <CircularProgress /> : <DeviceList devices={data} /> : null}
    </>
  )
}
