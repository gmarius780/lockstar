import React from 'react'
import Typography from '@material-ui/core/Typography'
import { Theme, makeStyles, createStyles } from '@material-ui/core/styles'

const useStyles = makeStyles((theme: Theme) => createStyles({
  root: {
    padding: theme.spacing(3),
  },
  logo: {
    display: 'block',
    margin: 'auto',
  },
  text: {
    textAlign: 'center',
    color: 'black',
  },
}))

export default () => {
  const classes = useStyles()

  return (
    <div className={classes.root}>
      <img alt="Lockstar" className={classes.logo} src={`${process.env.PUBLIC_URL}/images/icons/android-chrome-192x192.png`} />
      <Typography variant="h5" className={classes.text} noWrap>
        Lockstar
      </Typography>
    </div>
  )
}
